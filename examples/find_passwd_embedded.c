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

---------

This file demonstrates how llextfs can be used in the context of the firmware
of an embedded device.

**/

#define llextfs_printf uart_printf

#define LLEXTFS_USE_GLUE

#include "extfs.h"

unsigned int g_passwd_file_first_lba = 0;

void find_passwd_file() {
	if (g_passwd_file_first_lba) // If the LBA is known already, quit
		return;

	reset_extfs();

	struct Partition first_partition_info;
    if (!parse_partition(&first_partition_info, 0))  {
        uart_printf("MBR corrupt or missing");
		return;
    }
    print_partition_metadata(&first_partition_info);

	__g_partition_offset = first_partition_info.start_sector * SECTOR_SIZE;

	struct Superblock sblock;
    if (!parse_superblock(&sblock, 0))  {
        uart_printf("Superblock corrupt or missing");
        return;
    }
    print_superblock_metadata(&sblock);

    struct Superblock backup_sblock;
    for (int bg_nr = 0; bg_nr < sblock.bg_count; bg_nr++) {
        if (!(sblock.feature_ro_compat & 0x01) || bg_nr == 0 || bg_nr == 1 || isPowerOfP(bg_nr, 3) || isPowerOfP(bg_nr, 5) || isPowerOfP(bg_nr, 7)) {
            if (!parse_superblock(&backup_sblock, bg_nr)) {
                uart_printf("Superblock backup %i corrupt", bg_nr);
                return;
            }
        }
    }

	struct Inode passwd_inode;
    unsigned int passwd_file_inode_nr;

    if (!get_inode_for_path(&sblock, "/etc/passwd", &passwd_file_inode_nr)) {
        uart_printf("File not found");
		return;
    }

	uart_printf("Passwd file inode: %i", passwd_file_inode_nr);

    if (parse_inode(&sblock, &passwd_inode, passwd_file_inode_nr)) {
        print_inode_metadata(&passwd_inode);

		unsigned int db_nr = 0;
		unsigned int db_block_nr = 0;

		if (get_inode_data_block(&sblock, &passwd_inode, db_nr++, &db_block_nr)) {
			uart_printf("Passwd file first LBA: %i", (db_block_nr * (sblock.block_size / SECTOR_SIZE)) + first_partition_info.start_sector);

			g_passwd_file_first_lba = (db_block_nr * (sblock.block_size / SECTOR_SIZE)) + first_partition_info.start_sector;
		}
    }
}