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

This file demonstrates the glue to integrate llextfs in the firmware of an
embedded device. Modify this to your own needs as required.

**/

#ifdef LLEXTFS_USE_GLUE

#include "extfs.h"

#include "firmware.h"

unsigned int __g_partition_offset = 0;

unsigned int g_bd_buffer = 0;
unsigned int g_current_lba = -1;

void reset_extfs() {
	__g_partition_offset = 0;

	g_current_lba = -1;

	llextfs_printf("BD Buffer: %x", TEMP_BUF_ADDR);
}

// ftl.c:
UINT32 get_physical_address(UINT32 const lba, UINT32 const lpage_addr);

int load_lba(unsigned int lba) {
	if (g_current_lba != lba) {
		UINT32 lpage_addr	= lba / SECTORS_PER_PAGE;
		UINT32 sect_offset	= lba % SECTORS_PER_PAGE;

		UINT32 phys_page = get_physical_address(lba, lpage_addr); 

		if (!phys_page) {
			return 0;
		}

		UINT32 bank = phys_page / PAGES_PER_BANK;
		UINT32 row = phys_page % PAGES_PER_BANK;

		llextfs_printf("Load LBA: %i from bank %i row %i sector %i", lba, bank, row, sect_offset);

		nand_page_read(bank, row / PAGES_PER_BLK, row % PAGES_PER_BLK, TEMP_BUF_ADDR);
		flash_finish();

		g_bd_buffer = sect_offset;
		g_current_lba = lba;
	}

	return 1;
}

uint8_t read_disk_uint8(unsigned int offset) {
	unsigned int lba = offset / SECTOR_SIZE;
	unsigned int lba_offset = offset % SECTOR_SIZE;
	
	if (!load_lba(lba)) {
		return 0;
	}

	return read_dram_8(TEMP_BUF_ADDR + (g_bd_buffer * BYTES_PER_SECTOR) + lba_offset);
}

uint16_t read_disk_uint16(unsigned int offset) {
	unsigned int lba = offset / SECTOR_SIZE;
	unsigned int lba_offset = offset % SECTOR_SIZE;
	
	if (!load_lba(lba)) {
		return 0;
	}

	return (read_dram_8(TEMP_BUF_ADDR + (g_bd_buffer * BYTES_PER_SECTOR) + lba_offset + 1) << 8) + 
		   read_dram_8(TEMP_BUF_ADDR + (g_bd_buffer * BYTES_PER_SECTOR) + lba_offset);
}

uint32_t read_disk_uint32(unsigned int offset) {
	unsigned int lba = offset / SECTOR_SIZE;
	unsigned int lba_offset = offset % SECTOR_SIZE;
	
	if (!load_lba(lba)) {
		return 0;
	}

	return (read_dram_8(TEMP_BUF_ADDR + (g_bd_buffer * BYTES_PER_SECTOR) + lba_offset + 3) << 24) + 
		   (read_dram_8(TEMP_BUF_ADDR + (g_bd_buffer * BYTES_PER_SECTOR) + lba_offset + 2) << 16) + 
		   (read_dram_8(TEMP_BUF_ADDR + (g_bd_buffer * BYTES_PER_SECTOR) + lba_offset + 1) << 8) + 
		   read_dram_8(TEMP_BUF_ADDR + (g_bd_buffer * BYTES_PER_SECTOR) + lba_offset);
}

uint8_t read_partition_uint8(unsigned int offset) {
	return read_disk_uint8(__g_partition_offset + offset);
}

uint16_t read_partition_uint16(unsigned int offset) {
	return read_disk_uint16(__g_partition_offset + offset);
}

uint32_t read_partition_uint32(unsigned int offset) {
	return read_disk_uint32(__g_partition_offset + offset);
}

#endif