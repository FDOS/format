/*
// Program:  Format
// Version:  0.91w
// (0.90b/c - avoid saving info when inappropriate - Eric Auer 2003)
// (0.91d - handle non-standard FAT12/FAT16 formats, debugging: Eric Auer 2003)
// (0.91k - w/o Convert_Huge_To_Integers, preserving bad sector list - EA 2004)
// (0.91l - adds Restore_File_System() written by EA 2004)
// (0.91p - merged "NOT saving..." messages, better FAT32 support - EA 2004)
// (0.91t - avoid double 100% display - EA 2005)
// (0.91u - mmapsec is unsigned long, not unsigned long unsigned :-P - 2005)
// (0.91w - remove spurious semicolon from line 139 (ignored if statement)
// Written By:  Brian E. Reifsnyder
// Copyright:  2002-2005 under the terms of the GNU GPL, Version 2
// Module Name:  savefs.c
// Module Description:  Save File System Function
*/

#include <string.h>	/* strncmp */

#include "format.h"
#include "floppy.h"
#include "driveio.h"
#include "btstrct.h"
#include "savefs.h"
#include "hdisk.h"	/* Force_Drive_Recheck */
#include "userint.h"	/* Display_Percentage_Formatted */
#include "msghlpr.h"




/* write one sector somewhere ;-) 0.91k */
void MMapWrite(unsigned long mmapsec, unsigned long * mmapbuf);
void MMapWrite(unsigned long mmapsec, unsigned long * mmapbuf)
{
  memcpy((void *)&sector_buffer[0], (void *)mmapbuf, 512);
  if (debug_prog==TRUE) printf(" [map %lu] ", mmapsec);
  if (Drive_IO(WRITE, mmapsec, -1) != 0)
    printf(catgets(catalog, 23, 0, "Cannot write MIRROR MAP sector %lu - UNFORMAT spoiled!\n"),
      mmapsec);
}


char const * const properties_not_preserved[] = {
  "Filesystem properties will change, cannot preserve the\n",
  "(possibly empty) old bad cluster list. Use a surface scan\n",
  "tool or FORMAT /U if you want to update the bad cluster list.\n",
  NULL
};


/* Save the old file system for possible recovery with unformat */
/* Used for SafeFormat and MIRROR, as you might have guessed... */
/* *** WARNING: This only works if (...) the root directory follows    *** */
/* *** right after the 2nd FAT. This is NOT always true for FAT32!     *** */
/* Changed in 0.91d to handle != 2 FATs and != 1 reserved/boot sector...   */
/* Depends on the unformat data format whether this can be more flexible!  */
/* Changed in 0.91p to save more of FAT32 data area (-> root dir)          */
/* Changed in 0.91p to save all reserved sectors, not only 1 boot sector   */
/* New 0.91r: Overwriting used data clusters allowed only for SafeFormat   */
void Save_File_System(int overwrite)
{
  int bad_boot_sector = TRUE;
  int end_of_root_directory_flag;

  unsigned long mirror_map[512/4]; /* 0.91k: only 1 sector for ANY FAT type */

  unsigned pointer = 0;

  unsigned long offset_from_end;

  unsigned long destination_sector;
  unsigned long source_sector;

  unsigned long last_used_sector;	/* BadClustPreserve returns this */
  unsigned long mirror_beginning;
  unsigned long mirror_map_beginning;
  unsigned long mirror_map_size;
  unsigned long mirror_size;
  unsigned long mirror_map_sector;

  unsigned long number_of_bytes_in_mirror_map;
  unsigned long number_of_logical_sectors_on_drive;
  unsigned long number_of_root_directory_entries;

  unsigned long beginning_of_fat;
  unsigned long beginning_of_root_directory;

  unsigned long sectors_per_fat;
  unsigned long reserved_sectors; /* usually 1 - boot sector [0e]w */
  unsigned long number_of_fats;   /* usually 2, but can be 1 [10]b */
  unsigned long sectors_per_cluster; /* 0.91k */

  unsigned long end_of_root_directory;

#define MAXCHUNKSIZE 16	/* could use one huge_sector_buffer completely */
  unsigned long chunksize;	/* 0.91o optimization */
  unsigned long percentage = 0;

  int fat_type = param.fat_type; /* Save_File_System default, from def. BPB */

#define BSWord(ofs) ( (unsigned int *)(&sector_buffer[ofs]) )[0]
#define BSLong(ofs) ( (unsigned long *)(&sector_buffer[ofs]) )[0]

/* write one sector of the mirror map whenever the buffer gets full 0.91k */
#define MMapCheckWrite if (pointer == (512/4)) { \
    if ((debug_prog==TRUE) && (source_sector >= beginning_of_root_directory)) \
      printf("\n"); \
    MMapWrite(mirror_map_sector, &mirror_map[0]); \
    mirror_map_sector++; \
    pointer = 0; \
  }

  offset_from_end = (param.drive_type == HARD) ? 20 : 5;

  /* Get the boot sector, compute the FAT size, compute the root dir size,*/
  /* and get the end of the logical drive. */
  Drive_IO(READ,0,1);

  if ( (sector_buffer[510]==0x55) && (sector_buffer[511]==0xaa) )
    {
    bad_boot_sector = FALSE;

    if ( BSWord(0x0b) != 512 )
      {
      printf(catgets(catalog, 23, 1, "Not 512 bytes / sector. Cannot save UNFORMAT data.\n"));
      bad_boot_sector = TRUE;
      }

    number_of_fats = sector_buffer[0x10];
    if ( (number_of_fats < 1) || (number_of_fats > 2) )
      {
      printf(catgets(catalog, 23, 2, "Not 1 or 2 FAT copies. Cannot save UNFORMAT data.\n"));
      bad_boot_sector = TRUE;
      }

    reserved_sectors = BSWord(0x0e);
    if ((param.fat_type != FAT32) && (reserved_sectors != 1))
      {
        printf(catgets(catalog, 23, 3, "WARNING: Number of reserved / boot sectors is %u, not 1.\n"),
          reserved_sectors);
        if ((reserved_sectors < 1) || (reserved_sectors > 64))
          bad_boot_sector = TRUE;
      }
    if ((param.fat_type == FAT32) &&
        (reserved_sectors != parameter_block.bpb.reserved_sectors))
      {
        printf(catgets(catalog, 23, 4, "WARNING: Reserved sectors are %u but will be %u after format.\n"),
          reserved_sectors, parameter_block.bpb.reserved_sectors);
        if ((reserved_sectors < 1) || (reserved_sectors > 64)) /* no ; */
          bad_boot_sector = TRUE;        
      }

    number_of_root_directory_entries = BSWord(0x11);
    if ( ( (number_of_root_directory_entries == 0) ||
           (BSWord(0x16) == 0) ) /* FAT1x size zero? */
         && (fat_type != FAT32))
      {
        printf(catgets(catalog, 23, 5, " Must be FAT32, not %s!\n"),
          (fat_type == FAT12) ? "FAT12" : "FAT16");
        /* 0 root dir entries or FAT1x size in Save_File_System */

        nomirror_wimp: /* common bailout point avoids duplicated code (0.91p) */

        printf(catgets(catalog, 23, 6, " NOT saving unformat info, not preserving bad cluster list.\n"));
        return;
      }
    if ( (number_of_root_directory_entries != 0) && (fat_type == FAT32) )
      {
        printf(catgets(catalog, 23, 7, "WARNING: FAT32 with FAT1x style extra Root Directory???\n"));
        goto nomirror_wimp;	/* 0.91p */
      }

    number_of_logical_sectors_on_drive = BSWord(0x13);
    if (number_of_logical_sectors_on_drive==0)
      number_of_logical_sectors_on_drive = /* if it was 0, use 32bit value */
        BSLong(0x20);

    sectors_per_fat = BSWord(0x16);
    if (sectors_per_fat == 0)
      { /* FAT32 case: 32bit sectors per fat value */
        sectors_per_fat = BSLong(0x24);
        if (fat_type != FAT32)
          {
            printf(catgets(catalog, 23, 8, " Must be FAT32, not FAT1x!\n"));
            goto nomirror_wimp;	/* 0.91p */
          }
      }
    else
      { /* FAT1x case: 16bit sectors per fat value nonzero */
        if (number_of_root_directory_entries == 0)
          {
            printf(catgets(catalog, 23, 9, " FAT32 Root Directory but FAT1x FAT!\n"));
            goto nomirror_wimp;	/* 0.91p */
          }
        if (fat_type != ( (sectors_per_fat <= 12) ? FAT12 : FAT16 ))
          {
            printf(catgets(catalog, 23, 10, " %s size but supposed to be FAT32!\n"),
              (sectors_per_fat <= 12) ? "FAT12" : "FAT16");
            goto nomirror_wimp;	/* 0.91p */
          }
        /* FAT12 is max 12 sectors, FAT16 min 16 sectors. Of course this   */
        /* goes wrong if the FAT12 fat is bigger than needed. To be really */
        /* foolproof, one had to count clusters and compare to 4085...     */
      } /* FAT1x case: 16bit sectors per fat value nonzero */

    /* cluster size, geometry, media descriptor and hidden sectors are */
    /* not relevant for the UNFORMAT information */

    if ( ((number_of_root_directory_entries & 15) != 0) ||
         ( (param.fat_type != FAT32) && (number_of_root_directory_entries < 16) ) ||
         ( (param.fat_type == FAT32) && (number_of_root_directory_entries != 0) ) ||
         (sectors_per_fat < 1) ||
         (number_of_logical_sectors_on_drive < 200) )
      { /* not plausible root directory or FAT or drive size */
        printf(catgets(catalog, 23, 11, " Implausible Root Directory, FAT or drive size! Bad boot sector?\n"));
        bad_boot_sector = TRUE; /* -ea */
      } else {
        if ( ( 1 + sectors_per_fat + (number_of_root_directory_entries >> 4) +
          5 ) > ( number_of_logical_sectors_on_drive >> 1 ) ) /* estimate only */
          {
          printf(catgets(catalog, 23, 12, " Big FAT for little data? Bad boot sector?\n"));
          bad_boot_sector = TRUE;
          }
      } /* root directory an FAT and drive size was plausible */

      sectors_per_cluster = BSWord(0x0d) & 0xff;
      if (!sectors_per_cluster) sectors_per_cluster = 256;

      if (fat_type == FAT32)
        {
          beginning_of_root_directory = reserved_sectors +
#if 0
            ( (BSLong(0x2c)-2) * sectors_per_cluster ) + /* NOT USED YET */
#endif
            ( number_of_fats * (unsigned long)sectors_per_fat );

          /* new 0.91p: */
	  if (sectors_per_cluster < 32) /* first root cluster < 512 dir entries big? */
	    {
              end_of_root_directory = beginning_of_root_directory
                 + 32 - 1; /* just backup first 16k of data area */
            } else {
              end_of_root_directory = beginning_of_root_directory
                + sectors_per_cluster - 1; /* just backup ONE root dir cluster */
            }

          if (BSLong(0x2c) != 2)
            printf(catgets(catalog, 23, 13, "Root Directory NOT in 1st cluster, NOT saving it!\n"));
          /* would have to change some code below to save it properly... */
        }
      else
        {
          beginning_of_root_directory = reserved_sectors +
            (number_of_fats * (unsigned long)sectors_per_fat);
          end_of_root_directory = beginning_of_root_directory +
           (number_of_root_directory_entries/16) - 1;
           /* (last sector which is still PART of the root directory) */
       }

    /* Compute the locations of the second FAT  and  the root directory */
    /* ***  We use the LAST FAT - root directory follows after it!  *** */
    beginning_of_fat = ((unsigned long)sectors_per_fat * (number_of_fats-1))
      + reserved_sectors;

    } /* boot sector had 0x55 0xaa magic and looks sane */
    



  /* If the boot sector is not any good, don't save the file system. */
  if (bad_boot_sector==TRUE)
    {
    printf(catgets(catalog, 23, 14, " Drive looks unformatted, UNFORMAT information NOT saved.\n"));
    /* not preserving "existing" bad cluster list either, of course! */
    return;
    }

  if ((fat_type == param.fat_type) &&
      (reserved_sectors == parameter_block.bpb.reserved_sectors) &&
      (number_of_fats == parameter_block.bpb.number_of_fats) &&
      (sectors_per_cluster == BPB_SECTORS_PER_CLUSTER(parameter_block.bpb)) &&
      ( (sectors_per_fat & 0xffff) ==
        ( (fat_type == FAT32) ? parameter_block.xbpb.fat_size_low
                              : parameter_block.bpb.sectors_per_fat ) )
     )
    {
      /* *** printf(" Scanning existing FAT for bad cluster marks...\n"); */
      last_used_sector = BadClustPreserve(); /* uses param...bpb values */
      /* *** bad sectors are stored in our global bad sect list */
    }
  else
    {
      unsigned long scratch;
	  Print_Messages_With_Pauses(catalog, 24, properties_not_preserved);

      if (reserved_sectors != parameter_block.bpb.reserved_sectors)
        printf(catgets(catalog, 23, 15, "Number of reserved sectors differs: FOUND %lu / PLANNED %u.\n"),
          reserved_sectors, parameter_block.bpb.reserved_sectors);
      if (number_of_fats != parameter_block.bpb.number_of_fats)
        printf(catgets(catalog, 23, 16, "Number of FATs differs: FOUND %lu / PLANNED %hu\n"),
          number_of_fats, parameter_block.bpb.number_of_fats);
      if (sectors_per_cluster != BPB_SECTORS_PER_CLUSTER(parameter_block.bpb))
        printf(catgets(catalog, 23, 17, "Cluster size differs: FOUND %lu / PLANNED %hu (sectors)\n"),
          sectors_per_cluster, BPB_SECTORS_PER_CLUSTER(parameter_block.bpb));
      scratch = parameter_block.xbpb.fat_size_high;
      scratch <<= 16;
      scratch |= parameter_block.xbpb.fat_size_low;
      if (param.fat_type != FAT32)
        scratch = parameter_block.bpb.sectors_per_fat;
      printf(catgets(catalog, 23, 18, "FAT size differs: FOUND %lu %s / PLANNED %lu %s\n"),
        (BSWord(0x16)) ? BSWord(0x16) : BSLong(0x24),
        (BSWord(0x16)) ? "FAT1x" : "FAT32",
        scratch,
        (param.fat_type == FAT32) ? "FAT32" : "FAT1x");
    } /* filesystem parameter mismatch */




  printf(" Saving UNFORMAT information...\n");

  /* Compute the beginning sector of the mirror map and the size of */
  /* the mirror image.     */
  /* Always mirroring only ONE boot sector and ONE (the last) FAT.  */
#if 0 /* pre 0.91p */
  mirror_size = 1 + sectors_per_fat +
    ((fat_type != FAT32) ? (number_of_root_directory_entries/16)
                         : (sectors_per_cluster)); /* see above... */
#else
  mirror_size = reserved_sectors + sectors_per_fat +
    (1 + end_of_root_directory - beginning_of_root_directory);
#endif

  mirror_map_size = ( (mirror_size+(88/8)+63) / 64 );
    /* added +(88/8)+63, removed +1 (for better rounding) in 0.91d */
    /* 84+4 bytes are header/trailer, plus 8 bytes per mirrored sector! */
    /* each slot is 8 bytes, so there are 64 of them in each map sector */

  mirror_beginning = ((number_of_logical_sectors_on_drive-1) - mirror_size)
    - offset_from_end; /* -1 because last=num-1, correct? */
  mirror_map_beginning = mirror_beginning - mirror_map_size;
    /* select beginning to have space for mirror data, mirror map and offset */

  if (mirror_map_beginning <= last_used_sector)
    {
      if (!overwrite)
        {
          printf(catgets(catalog, 23, 19, "MIRROR data would overwrite used clusters. Aborting."));
          Exit(4,38);	/* no space for MIRROR */
        }
      else
        {
          printf(catgets(catalog, 23, 20, "SafeFormat: have to trash %lu used data sectors!\n"),
              1L + last_used_sector - mirror_map_beginning);
        }
    } /* SafeFormat may overwrite used clusters, but MIRROR may not */

  /* Write the mirror map pointer to the last sectors of the logical drive. */
  /* layout: 1. map... 2. boot 3. fat2... 4. root... 5. pointer */
  Clear_Sector_Buffer();

  ((unsigned long *)&sector_buffer[0])[0] = mirror_map_beginning;
  memcpy((void *)&sector_buffer[4], "AMSESLIFVASRORIMESAEP", 21);
    /* pease mirorsav.fil sesam (?) */ /* Add pointer sector flag */

  if (debug_prog==TRUE)
    {
      printf("[DEBUG]  Writing mirror map pointer to sector ->  %lu\n",
       ((number_of_logical_sectors_on_drive-1) - offset_from_end) );
       /* -1 correct??? */
      printf("[DEBUG]  Mirror map will start at sector ->  %lu\n",
        mirror_map_beginning);
    }

  if (
    Drive_IO(WRITE, ((number_of_logical_sectors_on_drive-1) - offset_from_end),
      -1) != 0) /* -1 correct??? */
    {
      printf(catgets(catalog, 23, 21, "Mirror map pointer write error - UNFORMAT will fail for you!\n"));
      printf(catgets(catalog, 23, 22, "Skipping UNFORMAT / mirror data backup step.\n"));
      Clear_Sector_Buffer(); /* just to be sure... */
      return; /* abort this SAVEFS attempt. Continue with CREATEFS. - 0.91n */
    }

  /* Create the mirror map and copy the file system to the mirror.  */
  Clear_Sector_Buffer();

  memset(mirror_map, 0, sizeof(mirror_map)); /* Clear mirror_map buffer */
  mirror_map_sector = mirror_map_beginning; /* first sector to be used */

  memcpy((void *)&mirror_map[0], "X:\\MIRROR.FIL",13);
  mirror_map[0] &= 0xffffff00UL; /* remove the "X" */
  mirror_map[0] |= param.drive_letter[0]; /* add the real drive letter */

  /* Main mirror map creation and copying loop follows:  */

  pointer = 84/4;
  number_of_bytes_in_mirror_map = 84;
  source_sector = 0;
  destination_sector = mirror_beginning;

  end_of_root_directory_flag = FALSE;

  do
    {
      if ( (source_sector>=reserved_sectors) /* "> 0" before 0.91p */ &&
           (source_sector<beginning_of_fat) )
        source_sector = beginning_of_fat;
        /* skip first FAT if 2 FATs present, skip additional res. sectors */

      if (debug_prog==TRUE)
        {
        if (source_sector <= beginning_of_fat) /* 0.91d */
          {
          if (source_sector < beginning_of_fat) /* (some) BOOT sector */
            {
            if (source_sector == 0) /* better messages (0.91p) */
              {
                printf("[DEBUG]  Mirroring BOOT sector %lu at sector %lu%s\n",
                  source_sector, destination_sector,
                  (reserved_sectors > 1) ? " (etc.)" : "");
              }
            if ( (reserved_sectors > 1) &&
                 (source_sector == reserved_sectors-1) ) /* new 0.91p */
                printf("[DEBUG]  Mirroring last RESERVED sector %lu at sector %lu\n",
                  source_sector, destination_sector);
              /* could change this and suppress multiple messages in FAT32 */
            }
          else /* beginning of (last) FAT */
            {
            printf("[DEBUG]  Mirroring FAT %lu sector %lu at sector %lu, ...\n",
              number_of_fats, source_sector, destination_sector);
            }
          }
        else /* after FATs */
          {
          if (source_sector == beginning_of_root_directory)
            {
            printf("done.\n");
            printf("[DEBUG]  Mirroring ROOT DIR sector %lu at sector %lu, ",
              source_sector, destination_sector);
            }
          else
            {
            /* 0.91l - less verbose. Even too unverbose now? */
            if (source_sector == (beginning_of_root_directory-1))
              printf("[DEBUG]  ... mirroring last FAT sector %lu at %lu, ",
                source_sector, destination_sector);
            }
          }
        } /* debug_prog */


      chunksize = 0; /* chunking loop added 0.91o for multi sector I/O */
      do {
	unsigned long perc_new = 100UL *
	  (source_sector + chunksize - beginning_of_fat) / 
	  (beginning_of_root_directory - beginning_of_fat);

        /* Enter mapping information into mirror map buffer */
        mirror_map[pointer] = source_sector + chunksize;
        pointer++;
        MMapCheckWrite;
        mirror_map[pointer] = destination_sector + chunksize;
        pointer++;
        MMapCheckWrite;
        chunksize++;

        if ((source_sector+chunksize) > end_of_root_directory)
          end_of_root_directory_flag = TRUE;
          /* changed >= into > in 0.91d (save ALL root dir sectors) */

        if ( (source_sector >= beginning_of_fat) &&
             (perc_new != percentage) &&
             (perc_new <= 100) /* do not count root dir progress */ )
          {
            percentage = perc_new;
            Display_Percentage_Formatted(percentage);
          } /* percentage display added 0.91o */

      } while ( (chunksize < (sizeof(huge_sector_buffer_0) >> 9)) &&
        (source_sector >= beginning_of_fat) &&
        (end_of_root_directory_flag != TRUE) );
      /* chunking loop: only use multi sector I/O in FAT and root dir, */
      /* which are assumed to be consecutive - not for boot sector!    */

      /* printf("%lu-[%lu]->%lu ", source_sector, chunksize, destination_sector); */

      /* Copy mirror image one sector at a time */
      if (Drive_IO(READ, source_sector, -(int)chunksize) != 0)
        printf(catgets(catalog, 23, 23, "Read error at sector %lu - UNFORMAT data damaged\n"),
          source_sector);

      if (Drive_IO(WRITE, destination_sector, -(int)chunksize) != 0)
        printf(catgets(catalog, 23, 24, "Write error at sector %lu - UNFORMAT data damaged\n"),
          destination_sector);

      source_sector += chunksize;
      destination_sector += chunksize;
      number_of_bytes_in_mirror_map += (chunksize << 3); /* chunksize * 8 */

    } while (end_of_root_directory_flag==FALSE);
    /* End of main mirror map creation and copying loop!  */

  /* if (debug_prog==TRUE) printf("done.\n"); */
  if (percentage != 100) Display_Percentage_Formatted(100);

  /* Write trailer in mirror map */

  mirror_map[pointer] = 0;
  pointer = 512/4; /* force writing */
  MMapCheckWrite;

  number_of_bytes_in_mirror_map += 4; /* add trailer - 0.91d */

  printf(catgets(catalog, 23, 25, " Mirror map is %lu bytes long, "), number_of_bytes_in_mirror_map);
  printf(catgets(catalog, 23, 26, " %lu sectors mirrored.\n"), (number_of_bytes_in_mirror_map - 88) / 8);

  return;

}

/* copy back MIRROR data, overwriting both(!) FATs, boot sector and root dir */
/* TODO: Optimize this to pool access when sectors n ... n+m are all copied  */
/*   to sectors x ... x+m: Just doing one read and one write of m sectors    */
/*   size saves a lot of disk seek time. The limit for m is the buffer size. */
void Restore_File_System(void)
{
  /* FAT32 FIXME: if used root directory data is longer than 1 cluster,  */
  /* not all of it will be mirrored, and the restored cluster chain will */
  /* probably point to some old data for the remaining clusters.         */

  unsigned long mirror_map_sector;
  unsigned long mirror_map[512/4]; /* 0.91k: only 1 sector for ANY FAT type */
  int pointer = 0; /* pointer into mirror_map */

  unsigned long number_of_logical_sectors_on_drive;

  unsigned long beginning_of_fat = 0;
  unsigned long sectors_per_fat = 0;
  unsigned char number_of_fats = 0; /* unsigned CHAR this time! */

  /* default (x)BPB already read using Set_Hard_Drive_Media_Parameters() */
  if (parameter_block.bpb.total_sectors)
    {
      number_of_logical_sectors_on_drive = parameter_block.bpb.total_sectors;
    }
  else
    {
      number_of_logical_sectors_on_drive = parameter_block.bpb.large_sector_count_high;
      number_of_logical_sectors_on_drive <<= 16;
      number_of_logical_sectors_on_drive |= parameter_block.bpb.large_sector_count_low;
    }

#ifndef BSWord
#define BSWord(ofs) ( (unsigned int *)(&sector_buffer[ofs]) )[0]
#define BSLong(ofs) ( (unsigned long *)(&sector_buffer[ofs]) )[0]
#endif

  Drive_IO(READ, (number_of_logical_sectors_on_drive-1) -
    ( (param.drive_type == HARD) ? 20 : 5 ), 1);

  if (strncmp((void *)&sector_buffer[4], "AMSESLIFVASRORIMESAEP", 21))
    {
      printf(catgets(catalog, 23, 27, "No MIRROR / UNDELETE data: Wrong magic.\n"));
      return;
    }
  mirror_map_sector = BSLong(0); /* read sector number of mirror map */
  pointer = 84/4;                /* start reading after headers! */

  Drive_IO(READ, mirror_map_sector, 1);
  memcpy((void *)&mirror_map[0], (void *)&sector_buffer[0], 512);

  sector_buffer[13] = 0;         /* force string termination */
  if (debug_prog==TRUE) printf("[DEBUG]  Mirror map at sector %lu: '%s'\n",
    mirror_map_sector, (char *)&sector_buffer[0]);
    /* mirror map should start with string "x:\mirror.fil" */

  if (debug_prog==TRUE) printf("[DEBUG]  First mirrored sector is %lu stored at %lu\n",
    mirror_map[pointer], mirror_map[pointer+1]);

  while (1)
    {
      unsigned long backup_sector;
      unsigned long original_sector;

      original_sector = mirror_map[pointer];
      pointer++;
      if (pointer == (512/4))	/* if beyond sector size */
        {
          mirror_map_sector++;	/* fetch next mirror map sector */
          if (debug_prog==TRUE)
            printf("\n[DEBUG]  Processing mirror map sector %lu.\n",
              mirror_map_sector);
          else
            printf("*");

          Drive_IO(READ, mirror_map_sector, 1);
          memcpy((void *)&mirror_map[0], (void *)&sector_buffer[0], 512);
          pointer -= (512/4);	/* pointer will be 0 now */
        }
      backup_sector = mirror_map[pointer];
      pointer++;
      /* printf("%lu->%lu ", backup_sector, original_sector); */

      if ((number_of_fats > 0) &&
          ( (original_sector == 0UL) || (backup_sector == 0UL) ))
        {
          printf(catgets(catalog, 23, 28, "\n End of mirror map. UNFORMAT done.\n"));
          if (param.fat_type == FAT32)
            {
              Drive_IO(READ, 0, 1);
              backup_sector = BSWord(0x32);
              if (backup_sector && (backup_sector < beginning_of_fat))
                {
                  printf(catgets(catalog, 23, 29, " Cloning boot sector into backup.\n"));
                  if (Drive_IO(WRITE, backup_sector, -1)) printf(catgets(catalog, 1, 6, "Failed.\n"));
                }
              else
                backup_sector = 0; /* no backups */
              original_sector = BSWord(0x30);
              if (original_sector && (original_sector < beginning_of_fat)
                && ((original_sector+backup_sector) < beginning_of_fat) )
                {
                  printf(catgets(catalog, 23, 30, " Invalidating filesystem info sector data.\n"));
                  Drive_IO(READ, original_sector, 1);
                  memset((void *)&sector_buffer[488], 0xff, 8);
                    /* set free_count and next_free long int values to -1 */
                    /* the OS will then re-initialize them on next reboot */
                  if (Drive_IO(WRITE, original_sector, -1)) printf(catgets(catalog, 1, 6, "Failed.\n"));
                  if (Drive_IO(WRITE, original_sector+backup_sector, -1))
                    printf(catgets(catalog, 1, 6, "Failed.\n"));
                }
            } /* FAT32 postprocessing */

          return;
        }

      if (Drive_IO(READ, backup_sector, -1))
        {
          printf(catgets(catalog, 23, 31, "*** Could not copy backup sector %lu to sector %lu ***\n"),
            backup_sector, original_sector);
        }
      else
        {
          if ( (original_sector == 0) &&
               ( (sector_buffer[0x10] > 7) || (sector_buffer[0x10] < 1) ) )
            {
              printf(catgets(catalog, 23, 32, "Boot sector would be overwritten by nonsense, 0 or > 7 FATs.\n"));
              printf(catgets(catalog, 1, 7, "Aborting.\n"));
              Exit(4,39); /* restore filesystem aborted: boot sector odd */
            } /* nonsense boot protection */

          if (Drive_IO(WRITE, original_sector, -1))
            printf(catgets(catalog, 23, 33, "*** Could not restore sector %lu ***\n"), original_sector);
          if (debug_prog==TRUE) printf(".");

          if ( (number_of_fats > 1) &&	/* recreate 2nd FAT copy if needed */
               (original_sector >= beginning_of_fat) &&
               ( (original_sector-beginning_of_fat) < (2*sectors_per_fat) ) )
            {
              unsigned long cloned_sector = original_sector;

              if (original_sector >= (beginning_of_fat+sectors_per_fat))
                {
                  cloned_sector -= sectors_per_fat; /* clone 2nd into 1st */
                  if (debug_prog==TRUE) printf("<");
                }
              else
                {
                  cloned_sector += sectors_per_fat; /* clone 1st into 2nd */
                  if (debug_prog==TRUE) printf(">");
                }

              if (Drive_IO(WRITE, cloned_sector, -1))
                printf(catgets(catalog, 23, 34, "*** Could not clone FAT sector %lu into sector %lu ***\n"),
                  original_sector, cloned_sector);

            } /* FAT cloning */

          if (original_sector == 0UL)
            {
              number_of_fats = sector_buffer[0x10]; /* should be or 1 2 */
              if ( /* (number_of_fats < 0) || */ (number_of_fats > 2))
                {
                  if (!number_of_fats) number_of_fats++;
                  if (number_of_fats > 2) number_of_fats = 2;
                  printf(catgets(catalog, 23, 35, "WARNING: %hu FAT copies requested, using %hu instead.\n"),
                    sector_buffer[0x10], number_of_fats);
                }
              beginning_of_fat = BSWord(0x0e);	/* FAT begins after reserved sectors */
              sectors_per_fat = BSWord(0x16);		/* FAT16 style */
              if (!sectors_per_fat)
                {
                  sectors_per_fat = BSLong(0x24);	/* FAT32 style */
                  if (param.fat_type != FAT32)
                    printf(catgets(catalog, 23, 36, "WARNING: UNFORMAT turns FAT1x drive into FAT32.\n"));
                    /* according to recorded boot sector */
                  param.fat_type = FAT32;	/* 0 FAT1x size: FAT32 I/O style needed in Restore_File_System */
                }
              else
                {
                  if (param.fat_type == FAT32)
                    printf(catgets(catalog, 23, 37, "WARNING: UNFORMAT turns FAT32 drive into FAT1x!?\n"));
                }

#if 0			/* delay recheck because of locking - 0.91q */
//	      Force_Drive_Recheck(); /* if we have just written the boot sector */
#endif

              printf(catgets(catalog, 23, 38, "\n Boot sector data: %hu FAT copies (offset %lu), %lu sectors per FAT\n"),
                number_of_fats, beginning_of_fat, sectors_per_fat);
            } /* boot sector updated */

        } /* READ worked */
      
    } /* while */

  /* return; */ /* never reached */
  
}
