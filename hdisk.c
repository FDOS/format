/*
// Program:  Format
// Version:  0.91t
// Written By:  Brian E. Reifsnyder
// (updates 0.90b ... 0.91j by Eric Auer 2003)
// (updates 0.91k ... Eric Auer 2004)
// (0.91t - better WinNT/2000/XP detection, message tuning - EA 2005)
// Copyright:  2002-2005 under the terms of the GNU GPL, Version 2
// Module Name:  hdisk.c
// Module Description:  Hard Drive Specific Functions
*/

#define HDISK


#include <stdlib.h>

#include "floppy.h"
#include "format.h"
#include "createfs.h"
#include "btstrct.h"
#include "driveio.h"	/* Exit(), 0.91r: exit and unlock */
#include "hdisk.h"


void Get_Device_Parameters()
{
  unsigned error_code = 0;

  parameter_block.bpb.bytes_per_sector = 0;		/* *** */
  parameter_block.bpb.sectors_per_cluster = 0;		/* *** */
  parameter_block.bpb.number_of_fats = 0;		/* *** */

  /* Get the device parameters for the logical drive */

  regs.h.ah=0x44;                     /* IOCTL Block Device Request          */
  regs.h.al=0x0d;                     /* IOCTL */
  regs.h.bl=param.drive_number + 1;
  regs.h.ch=0x08;                     /* Always 0x08...unless fs is FAT32                         */
  regs.h.cl=0x60;                     /* Get device parameters               */
  regs.x.dx=FP_OFF(&(parameter_block.query_flags));
  sregs.ds =FP_SEG(&(parameter_block.query_flags));
#if 0
//  parameter_block.use_current_bpb = 0;	/* bit 0 clear = we want the DEFAULT BPB only! */
//  parameter_block.use_track_layout_fields = 0;
//  parameter_block.all_sectors_same_size = 1;	/* bit 2 must be set */
//  parameter_block.reserved = 0;
#else
  parameter_block.query_flags = 4; /* bit 0 clear, bit 2 set */
#endif
  parameter_block.bpb.large_sector_count_low = 0;  /* RBIL suggests that this is not returned??? */
  parameter_block.bpb.large_sector_count_high = 0; /* RBIL suggests that this is not returned??? */
  parameter_block.media_type = 0; /* 1 for 3x0k in 1200k drive, else 0 !?? */
  parameter_block.bpb.sectors_per_fat = 0xffff; /* *** MARKER */
  intdosx(&regs, &regs, &sregs);

  error_code = regs.h.al;

  if ( (regs.x.cflag != 0) ||
       /* (parameter_block.bpb.sectors_per_fat == 0xffff) || *//* *** MARKER */
       (parameter_block.bpb.sectors_per_fat > 512) ) /* not plausible for FAT1x */
    {
      /* Max reasonable FAT16 size 256 sectors, min FAT32 size is 513 sectors */
      if (regs.x.cflag == 0)
        {
          printf("Win98? Default BPB *FAT1x* %u sectors/FAT, %u root size. FAT32 forced.\n",
            parameter_block.bpb.sectors_per_fat,
            parameter_block.bpb.root_directory_entries);
          parameter_block.bpb.sectors_per_fat = 0;        /* force FAT32 */
          parameter_block.bpb.root_directory_entries = 0; /* force FAT32 */
        }
      else
        {
          if (error_code == 0x0f) {
            printf("Invalid Drive! Aborting.\n");
            goto gdp_giving_up;
          }
          if (error_code == 0x05) {
            printf("Access Denied! LOCK problem? Aborting.\n");
            goto gdp_giving_up;
          }
          printf("GENIOCTL/0860 error %02x. Trying FAT32.\n",
            error_code);
        }

      parameter_block.bpb.sectors_per_fat = 0; /* faking FAT32 */
    } /* get FAT16 BPB failed or gave strange FAT1x size */

  if (parameter_block.bpb.sectors_per_fat == 0) /* FAT32, or flagged for FAT32? */
    { /* FAT32 stores 0 in this WORD and uses a DWORD elsewhere instead */
      /* Ok, this is assumed to be a FAT32 partition and it will be     */
      /* formatted as such.                                             */

    if (debug_prog==TRUE) printf("[DEBUG]  GDP: Looks like FAT32.\n");

    parameter_block.bpb.root_directory_entries = 0; /* ensure FAT32 */
    param.fat_type = FAT32; /* Get_Device_Parameters: 0 sect/FAT1x in def. BPB */
                                   /* ... or no def. BPB was available at all! */

    /* Get the device parameters for the logical drive */

    regs.h.ah=0x44;                     /* IOCTL Block Device Request          */
    regs.h.al=0x0d;                     /* IOCTL */
    regs.h.bl=param.drive_number + 1;
    regs.h.ch=0x48;                     /* Always 0x48 for FAT32                         */
    regs.h.cl=0x60;                     /* Get device parameters               */
    regs.x.dx=FP_OFF(&(parameter_block.query_flags));
    sregs.ds =FP_SEG(&(parameter_block.query_flags));
#if 0
//  parameter_block.use_current_bpb = 0;	/* bit 0 clear = we want the DEFAULT BPB only! */
//  parameter_block.use_track_layout_fields = 0;
//  parameter_block.all_sectors_same_size = 1;	/* bit 2 must be set */
//  parameter_block.reserved = 0;
#else
    parameter_block.query_flags = 4; /* bit 0 clear, bit 2 set */
#endif
    parameter_block.media_type = 0; /* 1 for 3x0k in 1200k drive, else 0 !?? */
    parameter_block.bpb.sectors_per_fat = 0xffff; /* *** new MARKER value  0.91L*/

    intdosx(&regs, &regs, &sregs);

    error_code = regs.h.al;

    if ( (regs.x.cflag != 0) ||
         (parameter_block.bpb.sectors_per_fat != 0) ) /* MUST be 0 for FAT32... 0.91m */
      {
        /* Add error trapping here */
        if (error_code == 0x0f) {
          printf("Invalid Drive! Aborting.\n");
          goto gdp_giving_up;
        }
        if (error_code == 0x05) {
          printf("Access Denied! LOCK problem? Aborting.\n");
          goto gdp_giving_up;
        }
        printf("GENIOCTL/4860 error %02x. No FAT32 kernel?\n", error_code);
        if (regs.x.cflag == 0)
          printf("FAT1x FAT size %u sectors?\n",
            parameter_block.bpb.sectors_per_fat);
        regs.x.ax = 0x3306;	/* get internal DOS version */
        regs.x.bx = 0xffff;	/* if 21.3306 not supported */
        intdos(&regs,&regs);	/* returns AL=FF if DOS version below 5.0 */
        /* if ((_osmajor == 5) && (param.drive_number > 1)) */
        if (regs.x.bx == 0x3205)	/* "DOS 5.50" revision DL, flags DH */
          printf("WinNT/XP/2k DOS box. Cannot format.\n");
        gdp_giving_up:
        Exit(4,55);		/* VARIOUS ways of G.D.P. to give up... */
      }

    if ((!parameter_block.xbpb.root_dir_start_high) &&
        (parameter_block.xbpb.root_dir_start_low < 2))
      { /* added 3 feb 2004 - Win98se dangerous problem fixed */
        printf("Corrected default BPB FAT32 root dir position to 2.\n");
        parameter_block.xbpb.root_dir_start_low = 2; /* first data cluster is #2 */
      }

    } /* FAT32 check */

  if ((parameter_block.bpb.sectors_per_fat > 0) && (parameter_block.bpb.sectors_per_fat <= 12))
    {
      if (param.fat_type == FAT32)
        printf("GDP self-correct: Must be FAT12! FAT1x size: %u\n",
          parameter_block.bpb.sectors_per_fat);
      if ((debug_prog==TRUE) /* && (param.fat_type != FAT12) */ )
        printf("[DEBUG]  FAT1x size: %u, using FAT12.\n",
          parameter_block.bpb.sectors_per_fat);
      param.fat_type = FAT12; /* Get_Device_Parameters: 1..12 FAT1x sectors */
    }
  else
    {
      if ((parameter_block.bpb.sectors_per_fat > 12) &&
          (parameter_block.bpb.sectors_per_fat < 512)) /* max useful would be 256 */
        {
          if (param.fat_type == FAT32)
            printf("GDP self-correct: Must be FAT16! FAT1x size: %u\n",
              parameter_block.bpb.sectors_per_fat);
          if ((debug_prog==TRUE) /* && (param.fat_type != FAT16) */ )
            printf("[DEBUG]  FAT1x size: %u, using FAT16.\n",
              parameter_block.bpb.sectors_per_fat);
          param.fat_type = FAT16; /* Get_Device_Parameters: > 12 FAT1x sectors */
        }
      else
        if (param.fat_type != FAT32)
          {
            printf("GDP self-correct: Is indeed FAT32! FAT size: %u\n",
              parameter_block.bpb.sectors_per_fat);
            param.fat_type = FAT32; /* Get_Device_Parameters: 0 FAT1x sectors */
          }
    } /* FAT1x size 0 or 13..64k sectors */

  if ((param.fat_type == FAT32) ||
      (parameter_block.bpb.sectors_per_fat == 0) ||
      (parameter_block.bpb.root_directory_entries == 0))
    {
      if ( (parameter_block.bpb.sectors_per_fat != 0) ||
           (parameter_block.bpb.root_directory_entries != 0) )
        {
          printf("GDP self-correct: Removing FAT1x FAT (%u) / root (%u) from FAT32 disk.\n",
            parameter_block.bpb.sectors_per_fat, parameter_block.bpb.root_directory_entries);
          parameter_block.bpb.sectors_per_fat = 0;	  /* G.D.P.: if one 0, other be 0 and type FAT32 */
          parameter_block.bpb.root_directory_entries = 0; /* G.D.P.: if one 0, other be 0 and type FAT32 */
        }
      if (param.fat_type != FAT32)
        printf("GDP self-correct: Cannot be FAT1x.\n");
      param.fat_type = FAT32;                             /* G.D.P.: if one 0, other be 0 and type FAT32 */
    } /* FAT32 */
  else
    {
      if (parameter_block.bpb.sectors_per_fat == 0)
        printf("GDP self-correct: Cannot be FAT1x, no FAT1x FAT\n");
      if (parameter_block.bpb.root_directory_entries == 0)
        printf("GDP self-correct: Cannot be FAT1x, no FAT1x root.\n");
      if ( (parameter_block.bpb.sectors_per_fat == 0) ||
           (parameter_block.bpb.root_directory_entries == 0) )
        {
          parameter_block.bpb.sectors_per_fat = 0;	  /* G.D.P. not FAT1x: if one 0, other be 0 and type FAT32 */
          parameter_block.bpb.root_directory_entries = 0; /* G.D.P. not FAT1x: if one 0, other be 0 and type FAT32 */
          param.fat_type = FAT32; /* FAT1x FAT or root dir: size 0 in Get_Device_Parameters */
        }
    } /* not FAT32 (or maybe FAT32 but not yet detected...) */

  if (error_code!=0)
    printf("GDP default BPB read error %02x.\n", error_code);

}


/* formerly called Set_DPB_Access_Flag, this function    */
/* forces re-creation of DPB when drive is next accessed */
void Force_Drive_Recheck()
{
  regs.h.ah = 0x0d; /* reset disk system of DOS, flush buffers */
  intdos(&regs, &regs);
  
  regs.h.ah = 0x32; /* force re-reading of boot sector (not useful for FAT1x?) */
  regs.h.dl = param.drive_number + 1;
  intdosx(&regs, &regs, &sregs);	/* DS:BX is DPB pointer on return - ignored! */
  segread(&sregs);			/* restore defaults */

  if (param.fat_type == FAT32)
    {
    /* structure is DW size 0x18, DW 0, DD function (2 "force media change"),
     * plus 0x10 unused bytes */
    union REGS r;
    struct SREGS s;
    char some_struc[0x20];
    memset(some_struc,0, 0x20);
    some_struc[0] = 0x18;
    some_struc[4] = 2;
    r.x.ax = 0x7304;		/* get/set FAT32 flag stuff */
    s.es   = FP_SEG(&some_struc[0]);
    r.x.di = FP_OFF(&some_struc[0]);
    r.h.dl = param.drive_number+1; /* A: is 1 etc. */
    r.x.cx = 0x18; /* structure size */
    intdosx(&r, &r, &s);
    segread(&sregs);		/* restore defaults */
    } /* FAT32 */
}


/*
 * Do everything to find a correct BPB and stuff.
 * Added extra sanity checks in 0.91e and later
 * *** FAT type detection improved, too. ***
 * Added alignment feature in 0.91m -ea
 */
void Set_Hard_Drive_Media_Parameters(int alignment)
{
  /* int index; */
  /* int result; */
  int i;

  unsigned long number_of_sectors;

  param.media_type = HD;
  param.fat_type = FAT12; /* Set_Hard_Drive_Media_Parameters before Get_Device_Parameters */

  Get_Device_Parameters(); /* read the default BPB etc., find FAT type */

  if ((param.fat_type == FAT32) && (alignment > 1)) /* new feature in 0.91m */
    {
      unsigned int mask = 3;

      if (parameter_block.bpb.number_of_fats & 1)
        {
          printf(" Align for odd number of FAT32 FATs.\n");
          mask = 7;
        } /* > 2 FATs will cause failure later, but at least THIS can handle it */

      if (parameter_block.bpb.reserved_sectors & 7) /* should not happen */
        {
          unsigned int howmuch = 8 - (parameter_block.bpb.reserved_sectors & 7);

          if (debug_prog==TRUE)
            printf("[DEBUG]  /A alignment to n*4k: adding %u reserved sectors.\n",
              howmuch);
          parameter_block.bpb.reserved_sectors += howmuch;
        }

      if (parameter_block.xbpb.fat_size_low & mask) /* does happen quite often */
        {
          unsigned int howmuch = 1 + mask - (parameter_block.xbpb.fat_size_low & mask);

          if (debug_prog==TRUE)
            printf("[DEBUG]  /A alignment to n*4k: adding %u sectors per FAT32 FAT.\n",
              howmuch);
          parameter_block.xbpb.fat_size_low += howmuch;

          if (parameter_block.xbpb.fat_size_low < 8) /* good to check, but... */
            parameter_block.xbpb.fat_size_high++;
            /* ... only FreeDOS and Win ME/XP can use > (16MB-64kB) per FAT FATs */
            /* (And only FreeDOS and Win XP (and ME?) can do 64k cluster sizes!) */
        }
    } /* alignment to make metadata 8*n sectors big */

  if (parameter_block.bpb.total_sectors!=0) /* 16bit value? */
    {
      number_of_sectors = parameter_block.bpb.total_sectors;
    }
  else /* else 32bit value */
    {
      number_of_sectors =  parameter_block.bpb.large_sector_count_high;
      number_of_sectors =  number_of_sectors<<16;
      number_of_sectors += parameter_block.bpb.large_sector_count_low;
      if (number_of_sectors == 0)
        {
          printf("Volume has no size!? Aborting.\n");
          Exit(4,56);
        }
    }

  if (parameter_block.bpb.bytes_per_sector != 512) /* SANITY CHECK */
    {
      printf("%d bytes / sector, not 512!? Aborting.\n",
        parameter_block.bpb.bytes_per_sector);
      Exit(4,57);
    }

  if ((parameter_block.bpb.number_of_fats<1) ||
      (parameter_block.bpb.number_of_fats>2)) /* SANITY CHECK */
    {
      printf("Not 1 or 2 FATs but %hu!? Aborting.\n",
        parameter_block.bpb.number_of_fats);
      Exit(4,58);
    }

  i = parameter_block.bpb.sectors_per_cluster;
  if (i == 128)
    {
      printf("WARNING: Cluster size is 64k. Will not work with Win9x or MS DOS!\n");
      printf("  WinME, WinNT/2k/XP/2003 and FreeDOS will be okay, though.\n");
    /* Examples of systems which do support 64k cluster size: FreeDOS and WinXP */
    }
  while ((i < 128) && (i != 0)) 
    i += i;	/* shift left until 64k / cluster */
  if (i != 128) /* SANITY CHECK */
    {
    i = parameter_block.bpb.sectors_per_cluster;
    printf("FATAL: Cluster size not 0.5, 1, 2, 4, 8, 16, 32 or 64k but %d.%dk!\n",
      i/2, (i & 1) ? 5 : 0);
    Exit(4,59);
    }

  Get_FS_Info(); /* create FS info from BPB */
  /* calculated things like FAT and cluster size in OLD versions (< 0.91j?) */
  /* start_fat_sector, number_fat_sectors, start_root_dir_sect (even FAT32) */
  /* number_root_dir_sect (even FAT32), total_clusters, all based on BPB    */
  /* Can modify root_dir_entries in BPB (FAT1x rounding), sets 0 for FAT32. */
  /* RUNS AGAIN when called from Create_File_System ... */

  if (file_sys_info.total_clusters > 65525UL)
    {
    if (param.fat_type!=FAT32)
      {
        printf(" Almost formatted FAT32 drive as FAT1x, phew...\n");
        param.fat_type = FAT32; /* Set_Hard_Drive_Media_Parameters cluster count fixup */
      }
    if (file_sys_info.total_clusters > (0x3fbc000UL-2))
      {
        printf("WARNING: FAT32 size will be more than (16 MB - 64 kB)!\n");
        printf("  Win9x will be unable to use the drive. Other OSes will use more RAM/CPU.\n");
      }
    }
  else
    {
    if (param.fat_type==FAT32)
      printf(" Almost formatted FAT1x drive as FAT32, phew...\n");
    param.fat_type = (file_sys_info.total_clusters>=4085UL) /* only if misdetected FAT32 */
      ? FAT16 : FAT12; /* Set_Hard_Drive_Media_Parameters cluster count fixup */
    }

  /* param.size = number_of_sectors/2048; unused? */

  /* space calculations improved 0.91h */
  drive_statistics.sect_total_disk_space  = number_of_sectors;

  /* changed 0.91i */
  drive_statistics.sect_available_on_disk = file_sys_info.total_clusters
    * parameter_block.bpb.sectors_per_cluster;

  if (param.fat_type == FAT32)
    drive_statistics.sect_available_on_disk -= file_sys_info.number_root_dir_sect;

  /* remove partial cluster after the last real cluster! - 0.91h */
  {
  unsigned long slack = drive_statistics.sect_available_on_disk;

  drive_statistics.sect_available_on_disk &=
    ~( (unsigned long) ( parameter_block.bpb.sectors_per_cluster - 1 ) );

  slack -= drive_statistics.sect_available_on_disk;

  if ( (debug_prog==TRUE) && (slack != 0) )
    printf("[DEBUG]  Partition contains %lu unused sectors after last cluster.\n",
      slack);

  } /* slack removal */

  drive_statistics.bytes_per_sector = parameter_block.bpb.bytes_per_sector;

  /* now "total" is "diskimage" size and "avail" is "free in data area" size. */

  drive_statistics.sect_in_each_allocation_unit =
    parameter_block.bpb.sectors_per_cluster;
   
  /* 0.91k - already tell the user what she has to expect size-wise */
  if (drive_statistics.bytes_per_sector == 512) {
    unsigned long roughsize;
    char sizeunit = 'k';
    roughsize = drive_statistics.sect_available_on_disk >> 1;
    if (roughsize > 9999) {
      roughsize += 512;
      roughsize >>= 10;
      sizeunit = 'M';
    }
    if (roughsize > 9999) { /* limit for LBA48 aware BIOS + LBA32 DOS: 2 TB */
      roughsize += 512;     /* limit for LBA28 BIOS: 128 GB */
      roughsize >>= 10;
      sizeunit = 'G'; /* Win9x has max 16 MB / FAT and 32k / clust: 128 GB */
    }
    printf(" Disk size: %lu %cbytes, ", roughsize, sizeunit); /* changed 0.91p */
  } else printf(" Warning: Disk has nonstandard sector size, "); /* same */

  printf("FAT%d. ***\n", /* changed 0.91p */
    (param.fat_type==FAT32) ? 32 : ( (param.fat_type==FAT16) ? 16 : 12 ) );

}
