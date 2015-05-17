/**

llextfs - Ext file system driver for low-level (embedded) systems

Copyright (c) 2015, Martijn Bogaard & Yonne de Bruijn
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include "extfs.h"

// Simple replacement as the embedded environment didn't provide a pow implementation
inline static int _pow(int x, int y) {
    int v = x;

    while (--y) {
        v *= x;
    }

    return v;
}

int parse_partition(struct Partition* partition, unsigned int partion_nr) {
    if (read_disk_uint16(0x1FE) != 0xAA55) { // Boot signature
        return 0;
    }

    if (!read_disk_uint8(0x1BE + (partion_nr * 16))) { // MBR partition status, partition active?
        return 0;
    }

    partition->start_sector = read_disk_uint32(0x1BE + (partion_nr * 16) + 8);
    partition->total_sectors = read_disk_uint32(0x1BE + (partion_nr * 16) + 12);

    return 1;
}

int parse_superblock(struct Superblock* sblock, unsigned int sb_nr) {
    const unsigned int sb_buffer_offset = (sb_nr > 0) ? ((sb_nr * sblock->bg_size) + 1024) : 1024;

    if (read_partition_uint16(sb_buffer_offset + 0x38) != 0xEF53) { // Magic signature
        return 0;
    }

    sblock->sb_nr = sb_nr;

    sblock->inode_count = read_partition_uint32(sb_buffer_offset); //0x0
    sblock->block_count = read_partition_uint32(sb_buffer_offset + 4);//0x4
    sblock->first_data_block = read_partition_uint32(sb_buffer_offset + 20); //0x14
    sblock->block_size = (unsigned int) _pow(2, (10 + read_partition_uint16(sb_buffer_offset + 24)));//0x18 2^(10+block_size) = actual block size
    sblock->blocks_per_group = read_partition_uint32(sb_buffer_offset + 32); //0x20
    sblock->inodes_per_group = read_partition_uint32(sb_buffer_offset + 40); //0x28
    sblock->signature = read_partition_uint16(sb_buffer_offset + 56);
    sblock->first_non_res_inode = read_partition_uint32(sb_buffer_offset + 84); //0x54
    sblock->inode_size = read_partition_uint16(sb_buffer_offset + 88);
    sblock->feature_ro_compat = read_partition_uint32(sb_buffer_offset + 0x64);

    sblock->bg_count = sblock->block_count / sblock->blocks_per_group + ((sblock->block_count % sblock->blocks_per_group) ? 1 : 0);
    sblock->bg_size = sblock->blocks_per_group * sblock->block_size;

    sblock->bg_desc_size = 32; //for 32b fs

    return 1;
}

void parse_bg_descriptor(struct Superblock* sblock, struct Blockgroup* bg_descriptor, unsigned int bg_nr) {
    const unsigned int bg_buffer_offset = (sblock->block_size <= 1024 ? 2 * sblock->block_size : sblock->block_size) + (bg_nr * sblock->bg_desc_size);

    bg_descriptor->bg_nr = bg_nr;

    bg_descriptor->inode_table_block_nr = read_partition_uint32(bg_buffer_offset + 8);
    bg_descriptor->block_bitmap_block_nr = read_partition_uint32(bg_buffer_offset + 0);
    bg_descriptor->inode_bitmap_block_nr = read_partition_uint32(bg_buffer_offset + 4);
}

int parse_inode(struct Superblock* sblock, struct Inode* inode, unsigned int inode_nr) {
    const unsigned int inode_bg_nr = (inode_nr - 1) / sblock->inodes_per_group;
    const unsigned int inode_nr_in_bg = (inode_nr - 1) % sblock->inodes_per_group;

    struct Blockgroup inode_bg;
    parse_bg_descriptor(sblock, &inode_bg, inode_bg_nr);

    const unsigned int inode_offset = (inode_bg.inode_table_block_nr * sblock->block_size) + (inode_nr_in_bg * sblock->inode_size);

    inode->inode_nr = inode_nr;
    inode->mode = read_partition_uint16(inode_offset);

    if (inode->mode > 0) {
        inode->in_use = 1;

        inode->uid = read_partition_uint16(inode_offset + 2);
        inode->size = read_partition_uint32(inode_offset + 4);
        inode->block_count = read_partition_uint32(inode_offset + 0x1C);
        inode->flags = read_partition_uint32(inode_offset + 0x20);

        if (inode->mode & 0x4000) {
            inode->filetype = FILETYPE_DIR;
        }
        else if (inode->mode & 0x8000) {
            inode->filetype = FILETYPE_FILE;
        }
        else {
            inode->filetype = FILETYPE_OTHER;
        }

        if (!(inode->flags & 0x80000 || inode->flags & 0x10000000)) { // Blockmap
            for (int i = 0; i < 15; i++) {
                inode->blockmap[i] = read_partition_uint32(inode_offset + 0x28 + (i * 4));
            }
        }

        return 1;
    }
    else {
        inode->in_use = 0;

        return 0;
    }
}

int get_inode_data_block(struct Superblock* sblock, struct Inode *inode, unsigned int db_nr, unsigned int* db_block_nr) {
    if (db_nr >= 12) { //indirect
        return 0;
    }
    else { //direct
        if (inode->blockmap[db_nr]) {
            *db_block_nr = inode->blockmap[db_nr];

            return 1;
        }
        else {
            return 0;
        }
    }
}

int get_inode_dirent(struct Superblock *sblock, struct Inode *inode, char file_name[], unsigned int* file_inode_nr, unsigned int *de_p) {
    unsigned int db_nr = *de_p / sblock->block_size;
    unsigned int db_offset = *de_p % sblock->block_size;

    if (*de_p >= inode->size) { //EOF
        return 0;
    }

    unsigned int db_block_nr;

    if (!get_inode_data_block(sblock, inode, db_nr, &db_block_nr)) {
        return 0;
    }

    unsigned int ent_inode = read_partition_uint32((db_block_nr * sblock->block_size) + db_offset);

    if (!ent_inode) { // Not in use
        return 0;
    }

    unsigned int ent_length = read_partition_uint16((db_block_nr * sblock->block_size) + db_offset + 4);
    unsigned short ent_name_length = read_partition_uint16((db_block_nr * sblock->block_size) + db_offset + 6) & 0xFF;

    if (!ent_length) { // Corrupt entry?
        return 0;
    }

    int p = 0;
    for (; p < ent_name_length; p++) {
        file_name[p] = read_partition_uint8((db_block_nr * sblock->block_size) + db_offset + 8 + p);
    }

    file_name[p] = 0;

    *file_inode_nr = ent_inode;
    *de_p += ent_length;

    return 1;
}

int get_inode_for_file_name_in_inode(struct Superblock* sblock, struct Inode* inode, char file_path[], unsigned int* file_inode_nr) {
    char file_name[FILE_PATH_SIZE] = {0};

    for (int i = 0; i < FILE_PATH_SIZE && file_path[i + 1] && file_path[i + 1] != '/'; i++) { // Get the first file from a path e.g. root from /root/dir1/dir2
        file_name[i] = file_path[i+1];
    }

	if (inode->filetype != FILETYPE_DIR) {
        return 0;
    }

    char current_file_name[FILE_PATH_SIZE];
    unsigned int current_file_inode_nr;

    unsigned int de_p = 0;
    while (get_inode_dirent(sblock, inode, current_file_name, &current_file_inode_nr, &de_p)) {
        if (strncmp(file_name, current_file_name, FILE_PATH_SIZE) == 0) {
            *file_inode_nr = current_file_inode_nr;

            return 1;
        }
    }

    return 0;
}

int get_inode_for_path(struct Superblock* sblock, char file_path[], unsigned int* file_inode_nr) {
    struct Inode root_inode;

    parse_inode(sblock, &root_inode, ROOT_DIR_INODE);

    struct Inode* inode_p = &root_inode;
    char* file_path_p = file_path;
    unsigned int current_file_inode_nr;

    while (file_path_p && *file_path_p) {
        if (!get_inode_for_file_name_in_inode(sblock, inode_p, file_path_p, &current_file_inode_nr)) {
            return 0;
        }

        parse_inode(sblock, inode_p, current_file_inode_nr);


        file_path_p = strstr(file_path_p + 1, "/");
    }

    *file_inode_nr = current_file_inode_nr;

    return 1;
}