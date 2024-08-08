/*
// Program:  Format
// Version:  0.91u
// (0.90b/c - fixed compiler warnings, pointers - Eric Auer 2003)
// 0.91b/c/d/e - fixed more bugs - Eric.
// 0.91i - using sizeof() instead of constants for memcpy - Eric.
// 0.91j - slight MS compatibility improvements - Eric.
// 0.91k ... Eric 2004
// 0.91u ... Eric 2005
// Written By:  Brian E. Reifsnyder
// Copyright:  2002-2005 under the terms of the GNU GPL, Version 2
// Module Name:  createfs.c
// Module Description:  Functions to create the file systems.
*/

#define CREATEFS

#include <stdlib.h>
#include <string.h>	/* strlen, memcpy -ea */
#include <dos.h>	/* FP_OFF, FP_SEG */
#include <stdio.h>	/* printf */

/* #include "hdisk.h" */	/* Force_Drive_Recheck (not HERE - 0.91q) */
#include "userint.h"	/* Display_Percentage_Formatted */

void E_memcpy(unsigned char far *, char *, size_t);
void E_memcpy(unsigned char far * dst, char * src, size_t cnt)
{

  /*
  // printf("E_memcpy(%04x:%04x, %04x:%04x, %d)\n",
  //   FP_SEG(dst), FP_OFF(dst), FP_SEG(src), FP_OFF(src), cnt);
  */

  while (cnt-- > 0) {
      dst[0] = src[0];
      dst++;
      src++;
  }
}

#include "bootcode.h"
#include "format.h"
#include "floppy.h"
#include "driveio.h"
#include "btstrct.h"
#include "createfs.h"

void Clear_Sector_Buffer(void); /* -ea */

#define CFSp(a) (void far *)(a)

void Clear_Reserved_Sectors(void);
void Create_Boot_Sect_Start(void);
void Create_Boot_Sect_Ext(void);
void Create_FS_Info_Sector(void);
void Get_FS_Info(void);
void Write_Boot_Sectors(void);
void Write_FAT_Tables(void);
void Write_Root_Directory(void);

unsigned long Create_Serial_Number(void);

FSI file_sys_info;


void Create_File_System()
{
  unsigned int where = 0;
  Get_FS_Info(); /* (aborts is not 1 or 2 FATs, or if not 512 by/sect) */

#if 0
//  /* Flush DOS disk buffers. (why that?) */
//  regs.h.ah = 0x0d;
//  intdos(&regs,&regs);
#endif

  if (debug_prog==TRUE) printf("[DEBUG]  File System Creation\n");




  /* Create boot sector structures. */
  Create_Boot_Sect_Start();	/* the initial jump and OEM id */
  Create_Boot_Sect_Ext();	/* FAT id string, LABEL, ... */

  Clear_Sector_Buffer();

  /* *** Add Start of boot sector */
  E_memcpy(&sector_buffer[0], (char *)&bs_start, sizeof(STD_BS) ); /* 11 */
  where = sizeof(STD_BS);

  /* *** Add Standard BPB */
  E_memcpy(&sector_buffer[where], (void *)&parameter_block.bpb,
    sizeof(STD_BPB) ); /* was 35-11+1 */
  where += sizeof(STD_BPB);

  /* *** Add FAT32 BPB Extension, if applicable. */
  if (param.fat_type==FAT32)
    {
      if ((parameter_block.xbpb.ext_flags & 0x7e) != 0) { /* 0.91u */
        /* MSB set if only 1 FAT in use, then LSB are 0-based FAT */
        /* number 00-0f. If MSB is 0, all (both) FATs are used... */
        printf("FAT32 flags 0x%04x fixed to 0.\n",
          parameter_block.xbpb.ext_flags); /* would mean 3rd/higher FAT in use */
        parameter_block.xbpb.ext_flags = 0;
      }
      if ((parameter_block.xbpb.file_system_version & 0xfefe) != 0) { /* 0.91u */
        /* 0.0 is the normal version, everything above 1.1 is fishy */
        printf("FAT32 version 0x%04x forced to 0.\n",
          parameter_block.xbpb.file_system_version);
        parameter_block.xbpb.file_system_version = 0;
      }
      if ( (parameter_block.xbpb.info_sector_number == 0) ||
           (parameter_block.xbpb.backup_boot_sector == 0) ||
           (parameter_block.xbpb.info_sector_number == 0xffff) || /* new 0.91p */
           (parameter_block.xbpb.backup_boot_sector == 0xffff) ) /* new 0.91p */
        {
          printf("Backup Boot / FS Info Sector ");
          if (parameter_block.bpb.reserved_sectors >= 16 /* 32 */)
            {
              if ( (parameter_block.xbpb.info_sector_number == 0xffff) || /* new 0.91p */
                   (parameter_block.xbpb.backup_boot_sector == 0xffff) )
                { /* new 0.91p, shorter in 0.91q */
                  printf("forced on.\n");
                } else {
                  printf("default position invalid.\n");
                }
              parameter_block.xbpb.info_sector_number = 1; /* common value */
              parameter_block.xbpb.backup_boot_sector = 6; /* FreeDOS default */
              printf(" Using sector 6 and 1.\n");
              /* boot, fs info, 2nd boot, -, -, -, boot, fs info, 2nd boot */
            }
          else	/* if less than 16 reserved sectors, accept "disable" state */
            {
              printf("disabled (no space).\n");
              parameter_block.xbpb.info_sector_number = 0xffff;
              parameter_block.xbpb.backup_boot_sector = 0xffff;
            }
        } /* Win98se fix added 20 jan 2004 0.91L */

      E_memcpy(&sector_buffer[where], (void *)&parameter_block.xbpb,
        sizeof(FAT32_BPB) ); /* was [36] and 63-36+1 */

      where += sizeof(FAT32_BPB);
    }

  /* *** Add Extended boot sector fields. */
  E_memcpy(&sector_buffer[where], (void *)&bs_ext,
    sizeof(EXT_BS) ); /* was ( [36] or [64] ) and ( 61-36+1 or 89-64+1 ) */
  where += sizeof(EXT_BS);

  /* Add Boot Code (at offset 62 or 90 probably...) */
  E_memcpy(&sector_buffer[where], (void *)&boot_code[0], sizeof(boot_code));
  where += sizeof(boot_code);

  strcat((char *)&sector_buffer[where], &boot_message[0]);

  /* Add Flag */
  sector_buffer[510]=0x55;
  sector_buffer[511]=0xaa;

  Write_Boot_Sectors();		/* This is the point of no return! */


  
#if 0	/* YES, this is too early, caused a sharing violation - 0.91q */
//  Force_Drive_Recheck();	/* or is this too early??? - 0.91k */
#endif

  /* Clear the OTHER reserved sectors, if any */
  if (parameter_block.bpb.reserved_sectors > 1)
    Clear_Reserved_Sectors();	/* spare the FAT32 stuff, of course */

  Write_FAT_Tables();		/* if FAT32, make root dir chain as well */

  Write_Root_Directory();	/* write root directory and label */

#if 0	/* this has to wait until we release the locks - 0.91q */
//  Force_Drive_Recheck();	/* Tell DOS that things have changed */
#endif

  if (debug_prog==TRUE) printf("[DEBUG]  File System Created\n");
}


void Create_Boot_Sect_Start()
{
  /* Add Jump Instruction */
  if(param.fat_type==FAT12 || param.fat_type==FAT16)
    {
    /* Add instruction for FAT12 & FAT16 */
    bs_start.jmp[0]=0xeb; /* Jump short to right after STD_BPB + EXT_BS: */
    bs_start.jmp[1]=0x3c; /* interesting enough, Windoze insists that we */
    bs_start.jmp[2]=0x90; /* use this entry point, or it won't trust us! */
    }
  else
    {
    /* Add instruction for FAT32 */
    bs_start.jmp[0]=0xeb;
    bs_start.jmp[1]=0x58;
    bs_start.jmp[2]=0x90;
    }

  /* Add OEM ID */
  memcpy(&bs_start.oem_id,"FRDOS4.1", 8);    /* new 0.91p, was FDOS_1.0 */
  /* OEM ID is 5 uppercase chars, digit, dot, digit, to please Windoze. */
  /* Win9x+ itself uses OEM IDs like "MSWIN4.1" ... */
  /* According to John Elliot, MS wants at least "?????3.3" */
}


void Create_Boot_Sect_Ext()
{
  unsigned long serial_number;

  /* Add Physical Drive Number. */
  if(param.drive_type==FLOPPY) bs_ext.drive_number=0x00;
  if(param.drive_type==HARD)   bs_ext.drive_number=0x80;

  /* Zero reserved field. */
  bs_ext.reserved=0x00;

  /* Add Signature Byte */
  bs_ext.signature_byte=0x29;

  /* Add Serial Number. */
  serial_number=Create_Serial_Number();
  memcpy(&bs_ext.serial_number[0], &serial_number, 4);

  memcpy(&drive_statistics.serial_number_low,&bs_ext.serial_number[0],2);
  memcpy(&drive_statistics.serial_number_high,&bs_ext.serial_number[2],2);

  /* Add Volume Label */
  if(param.v==TRUE)
    {
    memset( &bs_ext.volume_label[0], ' ', 11);
    memcpy( &bs_ext.volume_label[0], param.volume_label, min(11, strlen(param.volume_label)));
    }
  else
    {
    memcpy( &bs_ext.volume_label[0], "NO NAME    ", 11);
    }

  /* Add System ID */
  memcpy(&bs_ext.file_system_type[0],"FAT1",4);

  if (param.fat_type==FAT12) bs_ext.file_system_type[4]='2';
  if (param.fat_type==FAT16) bs_ext.file_system_type[4]='6';
  if (param.fat_type==FAT32) memcpy(&bs_ext.file_system_type[3],"32",2);

  memcpy(&bs_ext.file_system_type[5],"   ",3);
}


void Create_FS_Info_Sector()
{
  static struct
    {
    unsigned long    lead_signature         ;
    char             reserved_1        [480];
    unsigned long    struc_signature        ;
    unsigned long    free_count             ;
    unsigned long    next_free              ;
    char             reserved_2         [12];
    unsigned long    trailing_signature     ;
    } fs_info_sector;

  unsigned i;
  char far * fsIS_p = (char far *)(&fs_info_sector);

  for (i=0; i<sizeof(fs_info_sector); i++) {
    fsIS_p[i] = 0;
  }    
  /* memset(CFSp(fsIS_p), 0, sizeof(fs_info_sector)); */ /* & changed -ea */

  fs_info_sector.lead_signature     = 0x41615252L; /* RRaA */
  fs_info_sector.struc_signature    = 0x61417272L; /* rrAa */

  fs_info_sector.free_count         = file_sys_info.total_clusters -
    ( file_sys_info.number_root_dir_sect
      / BPB_SECTORS_PER_CLUSTER(parameter_block.bpb) ); /* root dir consumes clusters! */

#if 0 /* more correct but not even MS FORMAT seems to use that value! */
  fs_info_sector.next_free          = 2UL + /* 2 based counting */
    ( file_sys_info.number_root_dir_sect
      / BPB_SECTORS_PER_CLUSTER(parameter_block.bpb) ); /* root dir consumes clusters! */
#else
  fs_info_sector.next_free          = 2UL;
#endif
  
  fs_info_sector.trailing_signature = 0xaa550000L;

  E_memcpy(&sector_buffer[0], (char *)(&fs_info_sector), 512);


}


/* divide 16 or 32 bit total size by cluster size and sub extra areas */
/* ... then divide by cluster size to get number of clusters on drive */
static unsigned long cluster_count(void)
{
  unsigned long totalsect;
  
  totalsect = parameter_block.bpb.total_sectors;
  if (totalsect == 0)
    {
    totalsect = parameter_block.bpb.large_sector_count_high;
    totalsect <<= 16;
    totalsect |= parameter_block.bpb.large_sector_count_low;
    }

  /* If FAT32, root dir does not have to start right after FAT, */
  /* HOWEVER, WE always put it there, so this is acceptable:    */
  totalsect -= file_sys_info.start_root_dir_sect; /* *** set by Get_FS_Info *** */

  if (param.fat_type != FAT32) /* in FAT32, root dir uses normal clusters */
    totalsect -= file_sys_info.number_root_dir_sect;

  return (totalsect / BPB_SECTORS_PER_CLUSTER(parameter_block.bpb));
}


/* calculate: start_fat_sector, number_fat_sectors,          */
/* start_root_dir_sect, number_root_dir_sect, total_clusters */
void Get_FS_Info()
{
  /* first is easy: FAT starts after boot sector(s). 1 for most FAT12  */
  /* and FAT16, and usually 32 or 64 for FAT32, of which only 2 copies */
  /* of 3 sectors each are used: loader1, statistics and loader2...    */
  file_sys_info.start_fat_sector = parameter_block.bpb.reserved_sectors;

  /* we sanitize this later! */
  file_sys_info.number_fat_sectors = parameter_block.bpb.sectors_per_fat;

  if (file_sys_info.number_fat_sectors == 0) /* FAT32 uses 32 bit instead */
    {
    param.fat_type = FAT32; /* we found FAT32 (Get_FS_Info, 0 fat1x sectors) */
    file_sys_info.number_fat_sectors = parameter_block.xbpb.fat_size_high;
    file_sys_info.number_fat_sectors <<= 16;
    file_sys_info.number_fat_sectors |= parameter_block.xbpb.fat_size_low;
    }
  /* (by definition - actually, FAT32 FATs can be only 16 MB each anyway) */
  /* ... at least Win9x has this limit. WinXP even uses 64k clust size... */

  /* FAT32 technically can have the root dir anywhere but WE need it HERE */
  /* Among others, cluster_count() needs it there, to be more exact.      */
  file_sys_info.start_root_dir_sect = file_sys_info.number_fat_sectors;
  file_sys_info.start_root_dir_sect *= parameter_block.bpb.number_of_fats;
  file_sys_info.start_root_dir_sect += parameter_block.bpb.reserved_sectors;

  if (param.fat_type != FAT32) /* FAT12 / FAT16 root dir size sanitizing */
    {
      unsigned entries_per_sector = parameter_block.bpb.bytes_per_sector / 32;

      if ( ( parameter_block.bpb.root_directory_entries &
            (entries_per_sector-1) ) != 0)
        {
        parameter_block.bpb.root_directory_entries |= entries_per_sector - 1; /* round up to... */
        parameter_block.bpb.root_directory_entries++;	/* ...next multiple */
        printf("Rounded root dir size to %u entries, multiple of %u.\n",
        parameter_block.bpb.root_directory_entries, entries_per_sector);
        } /* not multiple of (512/32) (bug before 0.91i: used multiple of 32) */

      file_sys_info.number_root_dir_sect = parameter_block.bpb.root_directory_entries
        / entries_per_sector;
    }
  else /* FAT32 root dir size magic - changed in 0.91i+ */
    {
      unsigned entries_per_cluster;
      unsigned long rdent = 512; /* default number of root dir entries */

      entries_per_cluster = parameter_block.bpb.bytes_per_sector / 32;
      entries_per_cluster *= BPB_SECTORS_PER_CLUSTER(parameter_block.bpb);

      parameter_block.bpb.root_directory_entries = 0; /* must be 0 for FAT32 */

      if (rdent < entries_per_cluster)
        rdent = entries_per_cluster; /* at least 1 cluster */
      
      file_sys_info.number_root_dir_sect =  (32UL * rdent)
        / parameter_block.bpb.bytes_per_sector;
    } /* FAT32 root dir sizing */


  /* divide total 16 or 32 bit sector count by cluster size, */
  /* but subtract FAT and BOOT sizes first. FAT32 only works */
  /* in the current version if root dir starts in 1. cluster */
  /* *** Needs valid file_sys_info which is set above.   *** */
  /* *** This is the only call to cluster_count() in 0.91i. *** */
  file_sys_info.total_clusters = cluster_count();

  if (parameter_block.bpb.bytes_per_sector != 512)
   {
   printf("Not 512 bytes per sector. Aborting!\n");
   Exit(4,21);
   }
  if (  (parameter_block.bpb.number_of_fats<1)
     || (parameter_block.bpb.number_of_fats>2) )
   {
   printf("Invalid FAT count. Aborting!\n");
   Exit(4,21);
   }

}


/* clear all UNUSED reserved sectors */
void Clear_Reserved_Sectors()
{
  unsigned int index;
  unsigned int fsinfo = 0;
  unsigned int backup = 0;

  if (param.fat_type == FAT32)
    {
      fsinfo = parameter_block.xbpb.info_sector_number;
      if (fsinfo > parameter_block.bpb.reserved_sectors)
        fsinfo = 0; /* fold over boot sect if none */
      backup = parameter_block.xbpb.backup_boot_sector;
      if (backup > parameter_block.bpb.reserved_sectors)
        backup = 0; /* make backup offset 0 if none */
    }

  Clear_Sector_Buffer();
  index = (param.fat_type == FAT32) ? 3 : 1; /* spare the boot sector(s) */
  if (debug_prog==TRUE) printf("[DEBUG]  Zapping -> ");

  do
    {
      if ( (param.fat_type != FAT32) || (index != fsinfo) )
         /* only spare 1 (fat32: 3) boot sector(s) and if fat32 fsinfo */
         /* fat32 is 0 boot, 1-or-x fsinfo, 2 boot2... can boot2 move? */
        {
           if ( ( (index >= backup) && (param.fat_type == FAT32) ) &&
                ( ((index-backup) < 3) || ((index-backup) == fsinfo) )
              ) /* ... and spare their backups */
             {
             /* spare backup copies */
             }
           else
             {
               if (debug_prog==TRUE) printf("%u ", index);
               if (Drive_IO(WRITE, index, -1) != 0)
                 printf("(zap %u failed) ", index);
             }
        }
      index++;
    } while (index < parameter_block.bpb.reserved_sectors);

  if (debug_prog==TRUE) printf("done.\n");
}


unsigned long Create_Serial_Number()
{
  unsigned long serial_number;
  unsigned long serial_number_high;
  unsigned long serial_number_low;
  unsigned long scratch;

  unsigned char month;
  unsigned char day;
  unsigned int year;

  unsigned char hour;
  unsigned char minute;
  unsigned char second;
  unsigned char hundredth;

/* more compatible than using Turbo C getdate / gettime or          */
/* Watcom C _dos_getdate / _dos_gettime, but less elegant.            */
/* Turbo C: struct time ti, struct date da, getdate(&da), gettime(&ti), */
/* year = da.da_year; day = da.da_day; month = da.da_mon; hour = ti.      */
/* ti_hour; minute = ti.ti_min; second = ti.ti_sec; hundredth = ti.ti_hund; */
/* Watcom C: struct dostime_t ti, struct dosdate_t da, _dos_getdate(&da), */
/* _dos_gettime(&ti), year=da.year; day=da.day; month=da.month; hour=ti. */
/* hour; minute = ti.minute; second = ti.second; hundredth=ti.hsecond;  */
/* Assembly: ah=2a int 21 year=cx month=dh day=dl                      */
/* Assembly: ah=2c int 21 hour=ch minute=cl second=dh hundredth=dl    */

  regs.h.ah = 0x2a;
  intdos(&regs, &regs);
  year = regs.x.cx;
  month = regs.h.dh;
  day = regs.h.dl;

  regs.h.ah = 0x2c;
  intdos(&regs, &regs);
  hour = regs.h.ch;
  minute = regs.h.cl;
  second = regs.h.dh;
  hundredth = regs.h.dl;

  serial_number_low = hour;
  serial_number_low = serial_number_low << 8;
  serial_number_low += minute;
  scratch = year;
  serial_number_low += scratch;

  serial_number_high = second;
  serial_number_high = serial_number_high << 8;
  serial_number_high += hundredth;
  scratch = month;
  scratch = scratch << 8;
  scratch += day;
  serial_number_high += scratch;

  serial_number = (serial_number_high << 16) + serial_number_low;

  return serial_number;
}


void Write_Boot_Sectors()
{
  unsigned int use_backups = 0;

  /* Write boot sector to the first sector of the disk */
  if (debug_prog==TRUE) printf("[DEBUG]  Boot Sector -> 0\n");

  Drive_IO(WRITE, 0, 1);

  if (param.fat_type==FAT32)
    {
    if ( (parameter_block.xbpb.backup_boot_sector<3) ||
         (parameter_block.xbpb.backup_boot_sector >
          (parameter_block.bpb.reserved_sectors-3)) ||
         (parameter_block.xbpb.backup_boot_sector==0xffff) )
      {
        if (parameter_block.xbpb.backup_boot_sector==0xffff)
          {
            printf("No Backup Boot Sector.\n");
          }
        else
          {		/* simplified message 0.91q */
            printf("Invalid Backup Boot Sector position %u??\n",
              parameter_block.xbpb.backup_boot_sector);
          }
      }
    else
      {
        use_backups = parameter_block.xbpb.backup_boot_sector; /* only if valid */
        if (debug_prog==TRUE)
          printf("[DEBUG]  Backup Boot Sector -> %u\n",
            use_backups);

        if (Drive_IO(WRITE, use_backups, -1) != 0) printf("Failed!\n");
      }

    /* This sector only contains a bit of statistics if enabled. */
    if ( ((parameter_block.xbpb.info_sector_number + use_backups) >=
          parameter_block.bpb.reserved_sectors) ||
          (parameter_block.xbpb.info_sector_number == 0xffff) ||
          (parameter_block.xbpb.info_sector_number < 1) || /* 0.91k */
          (parameter_block.xbpb.info_sector_number == 2)   /* 0.91L */
       )
      {
        if (parameter_block.xbpb.info_sector_number == 0xffff)
          {
            printf("No FS Info Sector.\n");
          }
        else
          {		/* simplified message 0.91q */
              printf("Invalid FS Info Sector position %u??\n",
                parameter_block.xbpb.info_sector_number);
          }
      }
    else /* info sector active */
      {

        Create_FS_Info_Sector();

        if (debug_prog) printf("[DEBUG]  FS Info Sector -> %u\n",
          parameter_block.xbpb.info_sector_number);

        Drive_IO(WRITE, parameter_block.xbpb.info_sector_number, 1);

        if (use_backups)
          {
            if (debug_prog) printf("[DEBUG]  Backup FS Info Sector -> %u\n",
              use_backups + parameter_block.xbpb.info_sector_number);

            /* The backup contains -1 (invalid) as free_count value! - 0.91j */
            sector_buffer[488] = 0xff;
            sector_buffer[489] = 0xff;
            sector_buffer[490] = 0xff;
            sector_buffer[491] = 0xff;

            if (Drive_IO(WRITE, use_backups + parameter_block.xbpb.info_sector_number,
              -1) != 0) printf("Failed!\n");
          }
      } /* Info Sector feature */

    /* ** Create 3rd boot sector (2nd boot loader stage dummy). */
    Clear_Sector_Buffer(); /* ??? */
    /* Add Flag (00 at bytes 508 and 509 belongs to flag, too) */
    sector_buffer[510]=0x55;
    sector_buffer[511]=0xaa;

    /* Write sectors for "2nd stage of boot loader". */

    if (debug_prog) printf("[DEBUG]  Dummy Stage 2 -> 2\n");

    Drive_IO(WRITE, 2, 1); /* 0 boot 1/? fsinfo 2 stage2 */
    if (use_backups)
      {
      if (debug_prog) printf("[DEBUG]  Backup Dummy Stage 2 -> %u\n",
        use_backups + 2);

      if (Drive_IO(WRITE, use_backups + 2, -1) != 0) printf("Failed!\n");
      }

    } /* FAT32 */
}


void Write_FAT_Tables()
{
  unsigned long sector;
  unsigned long last_fatn_sector;
  unsigned long buf_sectors = sizeof(huge_sector_buffer_0) >> 9;
  unsigned long last_percentage = 999;

  /* Configure FAT Tables */
  Clear_Sector_Buffer();
  Clear_Huge_Sector_Buffer(); /* speed up things, use multisector I/O */

  sector = file_sys_info.start_fat_sector;
  last_fatn_sector = file_sys_info.number_fat_sectors;
  last_fatn_sector *= parameter_block.bpb.number_of_fats;
  last_fatn_sector += sector - 1;
  if ( (parameter_block.bpb.number_of_fats != 1) &&  /* should have been checked */
       (parameter_block.bpb.number_of_fats != 2) )   /* before anyway... */
    {
    printf("Invalid FAT count. Aborting!\n");
    Exit(4,22);
    }

  if ( file_sys_info.number_fat_sectors > 0x7F80L )  /* (16MB-64k)/512 */
    { /* equivalent to 0x3FC000-2 clusters. 128 GB at 32k cluster size */
    printf("WARNING: Each FAT is %lu sectors, > 16MB-64k, Win9x incompatible!\n",
      file_sys_info.number_fat_sectors); /* Bad for Win9x/ME, okay for others */
    /* WinXP can have > 128 GB LBA48 partitions and 64k cluster size...    */
    /* FreeDOS and Win2000 and WinNT(FAT16) support 64k cluster size, too. */
    /* Only Win2000 / WinME ever create > 128 GB FAT. WinXP FORMAT blocks > 32 GB */
    }

  if (debug_prog==TRUE)
    printf("[DEBUG]  FAT Sectors: %lu to %lu ->\n",
      file_sys_info.start_fat_sector,
      last_fatn_sector);
  else
    printf(" Preparing FAT area...\n");

  do
    {

    if (/* (debug_prog==TRUE) */ TRUE)
      {
        /* printf("\r %5lu... ", sector); */
        unsigned long perc = 100UL * (sector - file_sys_info.start_fat_sector);
        perc /= (1 + last_fatn_sector - file_sys_info.start_fat_sector); /* 0.91u */
        if (perc != last_percentage)
          {
            last_percentage = perc;
            Display_Percentage_Formatted(perc);
          }
      }

    if ((sector + buf_sectors) < last_fatn_sector)
      {
      Drive_IO(WRITE, sector, (int)buf_sectors); /* starting with start_fat_sector */
      sector += buf_sectors;
      }
    else
      {
      Drive_IO(WRITE, sector, 1); /* one of the last few sectors */
      sector++;
      }
    } while (sector <= last_fatn_sector);

  if ( /* (debug_prog==TRUE) */ TRUE)
    {
      Display_Percentage_Formatted(100UL);
      printf("\n");
    }

  sector_buffer[0] = parameter_block.bpb.media_descriptor;
  sector_buffer[1] = 0xff;
  sector_buffer[2] = 0xff;

  if (param.fat_type==FAT16)
    sector_buffer[3] = 0xff;

  if (param.fat_type==FAT32)
    {
    int sbp, cnum;
    unsigned long nclust;

    sector_buffer[3] = 0x0f;

    sector_buffer[4] = 0xff;
    sector_buffer[5] = 0xff;
    sector_buffer[6] = 0xff;
    sector_buffer[7] = 0x0f;
    
    /* WE always start ROOT DIR CLUSTER CHAIN at 1st cluster! */
    /* ADDED CLUSTER CHAIN CREATION - 0.91e, fixed 0.91i+ */
    sbp = 8;
    nclust = file_sys_info.number_root_dir_sect;
    nclust /= BPB_SECTORS_PER_CLUSTER(parameter_block.bpb);
    printf(" Optimized initial Root Directory size: %lu clusters.\n",
      nclust);
    cnum = 2; /* first data cluster has number 2, not 0...: 2 based counting */

    for ( ; (nclust > 0) && (sbp <= 508) ; nclust--)
      {
      if (nclust == 1) /* last cluster of root dir? Write End Of Chain! */
        {
        sector_buffer[sbp]   = 0xff;	/* end of chain (>= 0x0ffffff8)   */
        sector_buffer[sbp+1] = 0xff;    /* MS uses 0x0fffffff for example */
        sector_buffer[sbp+2] = 0xff;
        sector_buffer[sbp+3] = 0x0f;
        }
      else
        {
        sector_buffer[sbp] = cnum+1;	/* 1 byte is enough here */
        sector_buffer[sbp+1] = 0;	/* +1 because we POINT to the */
        sector_buffer[sbp+2] = 0;	/* NEXT cluster in the chain! */
        sector_buffer[sbp+3] = 0;
        }
      sbp += 4;
      cnum++;
      if (sbp >= 508)
        printf("Root Directory clipped to 125 clusters!\n");
      } /* for loop for chain creation */
    } /* FAT32 */

  if (debug_prog==TRUE)
    printf("[DEBUG]  FAT1 Start -> %lu\n",
      file_sys_info.start_fat_sector);

  Drive_IO(WRITE, file_sys_info.start_fat_sector, 1);

  if (parameter_block.bpb.number_of_fats==2)
    {
    if (debug_prog==TRUE)
      printf("[DEBUG]  FAT2 Start -> %lu\n",
        file_sys_info.start_fat_sector + file_sys_info.number_fat_sectors);

    Drive_IO(WRITE, (file_sys_info.start_fat_sector
      + file_sys_info.number_fat_sectors), 1);
    } /* 2 FATs */
}


void Write_Root_Directory()
{
  int index;
  unsigned long root_directory_first_sector;
  unsigned long root_directory_num_sectors;
  int space_fill;
  union REGS regs;

  unsigned long sector;

  /* Clear Root Directory Area */
  Clear_Sector_Buffer();

  sector_buffer[0] = 0;  /* -ea */
  sector_buffer[32] = 0; /* -ea */

  sector = file_sys_info.start_root_dir_sect; /* *** set by Get_FS_Info *** */
  root_directory_first_sector = sector;
  root_directory_num_sectors = file_sys_info.number_root_dir_sect;

  if (debug_prog==TRUE) printf("[DEBUG]  Root Directory Sectors: %lu to %lu ...",
    root_directory_first_sector,
    root_directory_first_sector + root_directory_num_sectors - 1);

  do
    {
    /* if (debug_prog==TRUE) printf("\r %5lu ",sector); */

    Drive_IO(WRITE, sector, 1);
    sector++;
    } while (sector < (root_directory_first_sector + root_directory_num_sectors) );

  if (debug_prog==TRUE) printf(" done.\n");

  /* Add Volume Label to Root Directory */
  if (param.v==TRUE)
    {
    Clear_Sector_Buffer();
    sector = file_sys_info.start_root_dir_sect; /* *** set by Get_FS_Info *** */
    index = 0;
    space_fill = FALSE;
    do
      {
      if (param.volume_label[(index)]==0x00) space_fill = TRUE;

      if (space_fill==FALSE) sector_buffer[index] = param.volume_label[(index)];
      else sector_buffer[index] = ' ';

      index++;
      } while (index<=10);

    sector_buffer[0x0b] = 0x08; /* file attribute: "volume label */

    /* setting of label / file system creation date and time: new 0.91b */

    regs.h.ah = 0x2c; /* get time */
    intdos(&regs,&regs); /* now ch=hour cl=minute dh=second dl=subseconds */
    /* word at 0x16 is creation time: secs/2:5 mins:6 hours:5 */
    regs.x.ax = regs.h.dh >> 1;
    regs.x.bx = regs.h.cl;
    regs.x.ax |= regs.x.bx << 5;  /* or in the minute */
    regs.x.bx = regs.h.ch;
    regs.x.ax |= regs.x.bx << 11; /* or in the hour */
    sector_buffer[0x16] = regs.h.al;
    sector_buffer[0x17] = regs.h.ah;

    regs.h.ah = 0x2a; /* get date */
    intdos(&regs,&regs); /* now cx=year dh=month dl=day al=wday (0=sun) */
    /* word at 0x18 is creation date: day:5 month:4 year-1980:7 */
    regs.x.ax = regs.h.dl;
    regs.x.bx = regs.h.dh;
    regs.x.cx -= 1980; /* subtract DOS epoch year 1980 */
    regs.x.ax |= regs.x.bx << 5; /* or in the month */
    regs.x.ax |= regs.x.cx << 9; /* or in DOS year! */
    sector_buffer[0x18] = regs.h.al;
    sector_buffer[0x19] = regs.h.ah;
    

    if (debug_prog==TRUE)
      printf("[DEBUG]  Root, Label, Timestamp -> %lu\n",sector);

    Drive_IO(WRITE, sector, 1);
    
    } /* if setting label */
}
