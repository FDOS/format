/*
// Program:  Format
// Version:  0.90
// Written By:  Brian E. Reifsnyder
// Copyright:  2002 under the terms of the GNU GPL, Version 2
// Module Name:  createfs.h
// Module Description:  Header file for file system creation module.
*/

#ifdef CREATEFS
  #define CFEXTERN /* */
#else
  #define CFEXTERN extern
#endif


typedef struct FS_Info
{
  unsigned long start_fat_sector;
  unsigned long number_fat_sectors;
  unsigned long start_root_dir_sect;
  unsigned long number_root_dir_sect;
  unsigned long total_clusters;
} FSI;

extern FSI file_sys_info;

CFEXTERN void Create_File_System(void);
CFEXTERN void Get_FS_Info(void);




