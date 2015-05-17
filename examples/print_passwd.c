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

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "extfs.h"

inline static int isPowerOfP(int n, int p) {
    if (n == 0)
        return 0;

    while (n % p == 0) {
        n /= p;
    }
    return n == 1;
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("%s <image file>\n", argv[0]);
		exit(0);
	}

    struct stat img_stat_info;

    stat(argv[1], &img_stat_info); //stat to get the size

    FILE *fd = fopen(argv[1], "r");

    if (fd == NULL) {
        perror("Can't open file");
        exit(1);
    }

    __g_disk_buffer_start = malloc(img_stat_info.st_size);

    int pos = fseek(fd, SEEK_SET, 0);
    int nr = fread((void *) __g_disk_buffer_start, img_stat_info.st_size, 1, fd);

    if (nr == -1) {
        perror("Can't read file");
        exit(1);
    }

    int ret = fclose(fd);

    if (ret == -1) {
        perror("Can't close file");
        exit(1);
    }

    struct Partition first_partition_info;
    if (!parse_partition(&first_partition_info, 0))  {
        printf("MBR corrupt or partition not used\n");
        return 0;
    }
    print_partition_metadata(&first_partition_info);

    __g_partition_buffer_start = __g_disk_buffer_start + first_partition_info.start_sector * SECTOR_SIZE;

    struct Superblock sblock;
    if (!parse_superblock(&sblock, 0))  {
        printf("superblock corrupt\n");
        return 0;
    }
    print_superblock_metadata(&sblock);

    struct Superblock backup_sblock;
    for (int bg_nr = 0; bg_nr < sblock.bg_count; bg_nr++) {
        if (!(sblock.feature_ro_compat & 0x01) || bg_nr == 0 || bg_nr == 1 || isPowerOfP(bg_nr, 3) || isPowerOfP(bg_nr, 5) || isPowerOfP(bg_nr, 7)) {
            if (!parse_superblock(&backup_sblock, bg_nr)) {
                printf("Superblock backup %i corrupt\n", bg_nr);
                return 0;
            }
        }
    }

    unsigned int passwd_file_inode;
    if (get_inode_for_path(&sblock, "/etc/passwd", &passwd_file_inode)) {
        printf("Passwd inode: %i\n", passwd_file_inode);
    }
    else {
        printf("File not found\n");
    }


    struct Inode passwd_inode;

    if (parse_inode(&sblock, &passwd_inode, passwd_file_inode)) {
        print_inode_metadata(&passwd_inode);

        printf("File content:\n");
        dump_inode_content(&sblock, &passwd_inode);
    }

    /*
    for (int bg_nr = 0; bg_nr < sblock.bg_count; bg_nr++) {
        struct Blockgroup bg_descriptor;

        parse_bg_descriptor(&sblock, &bg_descriptor, bg_nr);
        print_parsed_bg_descriptor(&bg_descriptor);
    }

    for (int inode_nr = 12241; inode_nr < sblock.inode_count; inode_nr++) {
        struct Inode inode;

        parse_inode(&sblock, &inode, inode_nr);
        print_inode_metadata(&inode);

        if (inode.filetype == FILETYPE_DIR) {
            char file_name[FILE_PATH_SIZE];
            unsigned int file_inode;

            unsigned int de_p = 0;
            while (get_inode_dirent(&sblock, &inode, file_name, &file_inode, &de_p)) {
                printf("Name: %s -> %i\n", file_name, file_inode);
            }
        }

        break;
    }
     */


    return 0;
}