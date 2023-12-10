/*
// Program:  Format
// Version:  0.91m
// Written By:  Brian E. Reifsnyder
// Copyright:  2002 under the terms of the GNU GPL, Version 2
// Module Name:  btstrct.h
// Module Description:  Boot sector structures header file.
*/

#ifndef BTSTRCT_H
#define BTSTRCT_H 1

#ifdef MAIN
#define BTEXTERN    /* */
#else
#define BTEXTERN    extern
#endif

#define BT8  unsigned char
#define BT16 unsigned int

typedef struct Standard_Boot_Sector_Fields
{
  BT8 jmp[3]   ;   /* 0x00   0 */
  BT8 oem_id[8];   /* 0x03   3 */
} STD_BS;

/* Helper macro to support cluster sizes of 256 sectors. This is encoded 
   as zero in bpb.sectors_per_cluster */
#define BPB_SECTORS_PER_CLUSTER(bpb)(((bpb).sectors_per_cluster) ? (bpb).sectors_per_cluster : 256)

typedef struct Standard_BPB
{
  BT16 bytes_per_sector       ;   /* 0x0b   11 */
  BT8  sectors_per_cluster    ;   /* 0x0d   13 */
  BT16 reserved_sectors       ;   /* 0x0e   14 */
  BT8  number_of_fats         ;   /* 0x10   16 */
  BT16 root_directory_entries ;   /* 0x11   17 */
  BT16 total_sectors          ;   /* 0x13   19 */
  BT8  media_descriptor       ;   /* 0x15   21 */
  BT16 sectors_per_fat        ;   /* 0x16   22 */
  BT16 sectors_per_cylinder   ;   /* 0x18   24 */
  BT16 number_of_heads        ;   /* 0x1a   26 */
  BT16 hidden_sectors_low     ;   /* 0x1c   28 */
  BT16 hidden_sectors_high    ;   /* 0x1e   30 */
  BT16 large_sector_count_low ;   /* 0x20   32 */
  BT16 large_sector_count_high;   /* 0x22   34 */
} STD_BPB;

typedef struct FAT32_BPB_Extension
{
  BT16 fat_size_low       ;   /* 0x24   36 */
  BT16 fat_size_high      ;   /* 0x26   38 */
  BT16 ext_flags          ;   /* 0x28   40 */
  BT16 file_system_version;   /* 0x2a   42 */
  BT16 root_dir_start_low ;   /* 0x2c   44 */
  BT16 root_dir_start_high;   /* 0x2e   46 */
  BT16 info_sector_number ;   /* 0x30   48 */
  BT16 backup_boot_sector ;   /* 0x32   50 */
  BT8    reserved_1[12]   ;   /* 0x34   52 */
} FAT32_BPB;

typedef struct Extended_Boot_Sector /* follows BPB or FAT32_BPB */
{
  BT8 drive_number       ;   /* varies */
  BT8 reserved           ;   /* varies */
  BT8 signature_byte     ;   /* varies */
  BT8 serial_number[4]   ;   /* varies */
  BT8 volume_label[11]   ;   /* varies */
  BT8 file_system_type[8];   /* varies */
} EXT_BS;


BTEXTERN STD_BS     bs_start;
BTEXTERN EXT_BS     bs_ext;

#endif
