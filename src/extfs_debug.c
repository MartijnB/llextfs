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

void print_partition_metadata(struct Partition *layout) {
    llextfs_printf("===================Partitions==================\n");
    llextfs_printf("start sector: %d\n", layout->start_sector);
    llextfs_printf("total amount of sectors: %d\n", layout->total_sectors);
}

void print_superblock_metadata(struct Superblock *sblock) {
    llextfs_printf("===================Superblock: %d==================\n", sblock->sb_nr);
    llextfs_printf("inode count: %d\n", sblock->inode_count);
    llextfs_printf("block count: %d\n", sblock->block_count);
    llextfs_printf("first data block: %d\n", sblock->first_data_block);
    llextfs_printf("block size: %d\n", sblock->block_size);
    llextfs_printf("blocks per group: %d\n", sblock->blocks_per_group);
    llextfs_printf("inodes per group: %d\n", sblock->inodes_per_group);
    llextfs_printf("magic signature: %d\n", sblock->signature);
    llextfs_printf("first non reserved inode: %d\n", sblock->first_non_res_inode);
    llextfs_printf("size of inode structure (bytes): %d\n", sblock->inode_size);
    llextfs_printf("block group count: %d\n", sblock->bg_count);
    llextfs_printf("block group size: %d\n", sblock->bg_size);
    llextfs_printf("block group desc size: %d\n", sblock->bg_desc_size);
}

void print_parsed_bg_descriptor(struct Blockgroup* bg_descriptor) {
    llextfs_printf("=================BG Descriptor: %d=================\n", bg_descriptor->bg_nr);
    llextfs_printf("block bitmap location: %d\n", bg_descriptor->block_bitmap_block_nr);
    llextfs_printf("inode bitmap location: %d\n", bg_descriptor->inode_bitmap_block_nr);
    llextfs_printf("inode table location: %d\n", bg_descriptor->inode_table_block_nr);
}

void print_inode_metadata(struct Inode *inode) {
    llextfs_printf("===================Inode: %d=================\n", inode->inode_nr);

    if (inode->in_use) {
        llextfs_printf("mode: %u\n", inode->mode);
        llextfs_printf("uid: %u\n", inode->uid);
        llextfs_printf("size: %u\n", inode->size);
        llextfs_printf("flags: %u\n", inode->flags);
        llextfs_printf("block count: %u\n", inode->block_count);

        if (inode->filetype == FILETYPE_DIR) {
            llextfs_printf("Filetype: Directory\n");
        }
        else if (inode->filetype == FILETYPE_FILE) {
            llextfs_printf("Filetype: File\n");
        }
        else {
            llextfs_printf("Filetype: Other\n");
        }

        for (int i = 0; i < 15; i++) {
            llextfs_printf("Block %i: %i\n", i, inode->blockmap[i]);
        }
    }
    else {
        llextfs_printf("Not used!\n");
    }
}

void dump_data_block(struct Superblock* sblock, unsigned int db_nr) {
    const unsigned int db_offset = db_nr * sblock->block_size;

    for (int p = 0; p < sblock->block_size; p++) {
        llextfs_printf("%c", read_partition_uint8(db_offset + p));
    }
}

void dump_inode_content(struct Superblock* sblock, struct Inode* inode) {
    unsigned int db_nr = 0;
    unsigned int db_block_nr = 0;

    while (get_inode_data_block(sblock, inode, db_nr++, &db_block_nr)) {
        dump_data_block(sblock, db_block_nr);
    }
}