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

#ifndef	EXTFS_H
#define	EXTFS_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SECTOR_SIZE 512

#define FILETYPE_FILE 1
#define FILETYPE_DIR 2
#define FILETYPE_OTHER 3

#define FILE_PATH_SIZE 256

#define ROOT_DIR_INODE 2

struct Partition {
    unsigned int start_sector;
    unsigned int total_sectors;
};

struct Superblock {
    unsigned int sb_nr;

    unsigned int inode_count; //0x0
    unsigned int block_count; //0x4
    unsigned int block_size; //0x18 2^(10+block_size) = actual block size
    unsigned int first_data_block; //0x14
    unsigned int blocks_per_group; //0x20
    unsigned int inodes_per_group; //0x28
    unsigned short signature; //0x38
    unsigned int first_non_res_inode; //0x54
    unsigned short inode_size;
    unsigned int feature_ro_compat;

    // Calculated:
    unsigned int bg_count;
    unsigned int bg_size;
    unsigned int bg_desc_size;
};

struct Blockgroup {
    unsigned int bg_nr;

    unsigned int inode_table_block_nr;
    unsigned int block_bitmap_block_nr;
    unsigned int inode_bitmap_block_nr;
};

struct Inode {
    unsigned int inode_nr;

    unsigned short in_use;

    unsigned short mode;
    unsigned short uid;
    unsigned int size;
    unsigned int flags;
    unsigned int block_count;

    unsigned short filetype;

    unsigned int blockmap[15];
};

extern unsigned int __g_partition_offset;

#ifndef llextfs_printf
    #include <stdio.h>
    #define llextfs_printf printf
#endif

#ifdef LLEXTFS_USE_GLUE
    void reset_extfs();

    uint8_t read_disk_uint8(unsigned int offset);
    uint16_t read_disk_uint16(unsigned int offset);
    uint32_t read_disk_uint32(unsigned int offset);

    uint8_t read_partition_uint8(unsigned int offset);
    uint16_t read_partition_uint16(unsigned int offset);
    uint32_t read_partition_uint32(unsigned int offset);
#else
    #define reset_extfs()

    const void* __g_disk_buffer_start;
    const void* __g_partition_buffer_start;

    #define read_disk_uint8(offset) (*(uint8_t *) (__g_disk_buffer_start + offset))
    #define read_disk_uint16(offset) (*(uint16_t *) (__g_disk_buffer_start + offset))
    #define read_disk_uint32(offset) (*(uint32_t *) (__g_disk_buffer_start + offset))

    #define read_partition_uint8(offset) (*(uint8_t *) (__g_partition_buffer_start + offset))
    #define read_partition_uint16(offset) (*(uint16_t *) (__g_partition_buffer_start + offset))
    #define read_partition_uint32(offset) (*(uint32_t *) (__g_partition_buffer_start + offset))
#endif

int parse_partition(struct Partition* partition, unsigned int partion_nr);
int parse_superblock(struct Superblock* sblock, unsigned int sb_nr);
void parse_bg_descriptor(struct Superblock* sblock, struct Blockgroup* bg_descriptor, unsigned int bg_nr);
int parse_inode(struct Superblock* sblock, struct Inode* inode, unsigned int inode_nr);

int get_inode_data_block(struct Superblock* sblock, struct Inode *inode, unsigned int db_nr, unsigned int* db_block_nr);
int get_inode_dirent(struct Superblock *sblock, struct Inode *inode, char file_name[], unsigned int* file_inode_nr, unsigned int *de_p);
int get_inode_for_file_name_in_inode(struct Superblock* sblock, struct Inode* inode, char file_path[], unsigned int* file_inode_nr);
int get_inode_for_path(struct Superblock* sblock, char file_path[], unsigned int* file_inode_nr);

void print_partition_metadata(struct Partition *layout);
void print_superblock_metadata(struct Superblock *sblock);
void print_parsed_bg_descriptor(struct Blockgroup* bg_descriptor);
void print_inode_metadata(struct Inode *inode);

void dump_data_block(struct Superblock* sblock, unsigned int db_nr);
void dump_inode_content(struct Superblock* sblock, struct Inode* inode);

#endif