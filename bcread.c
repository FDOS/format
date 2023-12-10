/*
// Program:    Format
// Version:    0.91t
// Written by: Eric Auer
// Copyright:  2004-2005 under the terms of the GNU GPL, Version 2
// Module Name:  bcread.c
// Module Description:  Read old FAT to find bad clusters
*/

#include "format.h"
#include "floppy.h"
#include "driveio.h"
#include "btstrct.h"
#include "savefs.h"
#include "userint.h" /* Display_Percentage_Formatted */

/* New in 0.91r: returns number of last used sector -ea */
unsigned long BadClustPreserve32(void);
unsigned long BadClustPreserve16(void);
unsigned long BadClustPreserve12(void);


int check_too_bad(unsigned int bad_count); /* new 0.91p */
int check_too_bad(unsigned int bad_count) /* return 1 if too many bad clusters */
{
	if (bad_count == MAX_BAD_SECTORS) {
          printf("\n *** Too many bad clusters! Do a surface scan after FORMAT! ***\n");
          return 1;
        }
        return 0;
} /* check_too_bad */


/*
 * Use param.bpb values and parse the existing FAT to fill the
 * global bad_sector_map table with already known bad sector numbers.
 * Used in Safe Quick format (/Q), called from savefs.c ...
 * Full format with surface scan (/U) and unsafe Quick format (/Q /U)
 * do not use this code. Extra in 0.91r: Returns max. used data sector number.
 */
unsigned long BadClustPreserve(void)
{
  if (parameter_block.bpb.bytes_per_sector != 512) {
    printf("BadClustPreserve aborted: not 512 bytes/sector!\n");
    return 0xffffffffUL;
  }
  if (param.fat_type == FAT32) {
    return BadClustPreserve32();
  } else {
    if (param.fat_type == FAT16) {
      return BadClustPreserve16();
    } else {
      return BadClustPreserve12();
    }
  }
}



unsigned long BadClustPreserve32(void) /* should use multisector read here */
{
  unsigned long fatstart = parameter_block.bpb.reserved_sectors;
  unsigned long fatsize;
  unsigned long cluststart = fatstart; /* where 1st cluster starts */
  unsigned long sect, j;
  unsigned long countused = 0;
  unsigned long countbad = 0;
  unsigned long countitems = 0;
  unsigned long * buf = (unsigned long *)(&huge_sector_buffer[0]);
  unsigned long chunksize = sizeof(huge_sector_buffer_0) >> 9;
  unsigned long percentage = 0;
  unsigned long last_used = 2;

  fatsize = parameter_block.xbpb.fat_size_high;
  fatsize <<= 16;
  fatsize |= parameter_block.xbpb.fat_size_low;
  cluststart += (fatsize * parameter_block.bpb.number_of_fats);

  if (debug_prog == TRUE)
    printf(" Scanning FAT Sectors %lu to %lu...\n",
      fatstart, fatstart+fatsize);
  else
    printf(" Scanning existing FAT...\n");

  j = 0;
  for (sect = fatstart; sect < (fatstart+fatsize); sect += chunksize) {
    unsigned int i;

    memset((void *)&buf[0], 0, (int)chunksize * 512);

    if ((sect + chunksize) > (fatstart+fatsize)) {
      chunksize = 1;	/* slow down in the end */
      buf = (unsigned long *)(&sector_buffer[0]);
      Drive_IO(READ, sect, 1);
    } else {
      Drive_IO(READ, sect, (int)chunksize);
    }

    if (/* debug_prog == */ TRUE)
      {
        /* printf(" Chunk at %8lu...\r", sect); */
        if ( percentage != (100UL * (sect-fatstart) / fatsize) )
          {
            percentage = 100UL * (sect-fatstart) / fatsize;
            Display_Percentage_Formatted(percentage);
          } /* using percentage, shorter log - 0.91o */
      }

    if (sect == fatstart) { /* ignore initial 2 entries */
      buf[0] = 0;
      buf[1] = 0;
    }

    for (i = 0; i < (chunksize * (512/4)); i++) {
      buf[i] &= 0x0fffffffUL; /* ignore 4 high bits */
      if ( (buf[i] < 0x0ffffff0UL) ||	/* free or used cluster */
           (buf[i] > 0x0ffffff7UL) ) {	/* last cluster of a chain */
        if (buf[i]) {
            countused++;		/* count to-be-deleted entries */
            last_used = j+i;
        }
        if (buf[i] > 0x0ffffff7UL)
            countitems++;		/* last cluster of 1 item */
      } else {
        if (check_too_bad(bad_sector_map_pointer)) {
          bad_sector_map_pointer--; /* too-many-bad-clusters workaround! */
        }
        bad_sector_map[bad_sector_map_pointer] =
          cluststart + ((j+i-2) * BPB_SECTORS_PER_CLUSTER(parameter_block.bpb));
        bad_sector_map_pointer++;
        countbad++;
      }
    } /* for slots in a sector */

    j += chunksize * 512/4; /* done with chunk */

  } /* for all FAT sectors */

  if (/* debug_prog == */ TRUE)
    {
      /* printf("\n"); */
      if (percentage != 100) Display_Percentage_Formatted(100);
    }

  printf("\n Cluster stats: %lu used, %lu bad, %lu items, %lu last.\n",
    countused, countbad, countitems, last_used);
  return ( cluststart +
    ( (1+last_used-2) * BPB_SECTORS_PER_CLUSTER(parameter_block.bpb) ) - 1 );
    /* we round up to the END of the cluster */
} /* BadClustPreserve32 */



unsigned long BadClustPreserve16(void)
{
  unsigned long fatstart = parameter_block.bpb.reserved_sectors;
  unsigned long fatsize = parameter_block.bpb.sectors_per_fat;
  unsigned long cluststart = fatstart; /* where 1st cluster starts */
  unsigned long sect, j;
  unsigned long countused = 0;
  unsigned long countbad = 0;
  unsigned long countitems = 0;
  unsigned int * buf = (unsigned int *)(&huge_sector_buffer[0]);
  unsigned long chunksize = sizeof(huge_sector_buffer_0) >> 9;
  unsigned long last_used = 2;

  cluststart += fatsize * parameter_block.bpb.number_of_fats;
  cluststart += (parameter_block.bpb.root_directory_entries+15) >> 4;
    /* * 32 / 512 -> 16 entries per sector */

  if (debug_prog == TRUE)
    printf(" Scanning FAT Sectors %lu to %lu...\n",
      fatstart, fatstart+fatsize);
  /* no scan announcement in non-debug mode: just show the results later. */

  j = 0;
  for (sect = fatstart; sect < (fatstart+fatsize); sect += chunksize) {
    unsigned int i;
    memset((void *)&buf[0], 0, (int)chunksize * 512);
    if ((sect + chunksize) > (fatstart+fatsize)) {
      chunksize = 1;	/* slow down in the end */
      buf = (unsigned int *)(&sector_buffer[0]);
      Drive_IO(READ, sect, 1);
    } else {
      Drive_IO(READ, sect, (int)chunksize);
    }
    if (debug_prog == TRUE)
      printf(" Chunk at %8lu...\r", sect);
    if (sect == fatstart) { /* ignore initial 2 entries */
      buf[0] = 0;
      buf[1] = 0;
    }
    for (i = 0; i < (chunksize * (512/2)); i++) {
      if ( (buf[i] < 0x0fff0UL) ||	/* free or used cluster */
           (buf[i] > 0x0fff7UL) ) {	/* last cluster of a chain */
        if (buf[i]) { 
            countused++;		/* count to-be-deleted entries */
            last_used = j+i;
        }
        if (buf[i] > 0x0fff7UL)
            countitems++;		/* last cluster of 1 item */
      } else {
        if (check_too_bad(bad_sector_map_pointer)) {
          bad_sector_map_pointer--; /* too-many-bad-clusters workaround! */
        }
        bad_sector_map[bad_sector_map_pointer] =
          cluststart + ((j+i-2) * BPB_SECTORS_PER_CLUSTER(parameter_block.bpb));
        bad_sector_map_pointer++;
        countbad++;
      }
    } /* for slots in a sector */
    j += chunksize * 512/2; /* done with chunk */
  } /* for all FAT sectors */
  if (debug_prog == TRUE) printf("\n");

  printf("\n Cluster stats: %lu used, %lu bad, %lu items, %lu last.\n",
    countused, countbad, countitems, last_used);
  return ( cluststart +
    ( (1+last_used-2) * BPB_SECTORS_PER_CLUSTER(parameter_block.bpb) ) - 1 );
    /* we round up to the END of the cluster */
} /* BadClustPreserve16 */



unsigned long BadClustPreserve12(void) /* FAT12 is max 12 sectors / FAT, simple */
{
  unsigned long fatstart = parameter_block.bpb.reserved_sectors;
  unsigned long fatsize = parameter_block.bpb.sectors_per_fat;
  unsigned long cluststart = fatstart; /* where 1st cluster starts */
  unsigned long countused = 0;
  unsigned long countbad = 0;
  unsigned long countitems = 0;
  unsigned int clust, index;
  unsigned char * buf = (unsigned char *)(&sector_buffer[0]);
  unsigned long last_used = 2;

  cluststart += fatsize * parameter_block.bpb.number_of_fats;
  cluststart += (parameter_block.bpb.root_directory_entries+15) >> 4;

  if (fatsize > 1) buf = (unsigned char *)(&huge_sector_buffer[0]);
  if (fatsize > (sizeof(huge_sector_buffer_0) >> 9)) {
    printf(" Cannot process existing FAT12, too big!\n");
    return 0xffffffffUL;
  }
  /* no scan announcement: just show the results. FAT12 is tiny. */

  Drive_IO(READ, parameter_block.bpb.reserved_sectors, (int)fatsize);
  index = 3;
  for (clust = 2; clust < 4080; clust+=2) {
    /* bytes 12 34 56 -> pair of clusters: first is 412, second is 563 */
    unsigned int clusta, clustb;
    clusta = buf[index+1] & 0x0f;
    clusta <<= 8;
    clusta |= buf[index];
    clustb = buf[index+2];
    clustb <<= 4;
    clustb |= buf[index+1] >> 4;
    if (clusta > 0xff7) {
      clusta = 1; /* end of chain */
      countitems++;
    }
    if (clustb > 0xff7) {
      clustb = 1; /* end of chain */
      countitems++;
    }
    if (clusta > 0xff0) {
      clusta = 0;
      countbad++;
      if (check_too_bad(bad_sector_map_pointer)) {
        bad_sector_map_pointer--; /* too-many-bad-clusters workaround! */
      }
      bad_sector_map[bad_sector_map_pointer] =
        cluststart + ((clust-2) * BPB_SECTORS_PER_CLUSTER(parameter_block.bpb));
      bad_sector_map_pointer++;
    }
    if (clustb > 0xff0) {
      clustb = 0;
      countbad++;
      if (check_too_bad(bad_sector_map_pointer)) {
        bad_sector_map_pointer--; /* too-many-bad-clusters workaround! */
      }
      bad_sector_map[bad_sector_map_pointer] =
        cluststart + ((clust+1-2) * BPB_SECTORS_PER_CLUSTER(parameter_block.bpb));
      bad_sector_map_pointer++;
    }
    if (clusta > 0) {
        countused++;
        last_used = clust;	/* first of a pair */
    }
    if (clustb > 0) {
        countused++;
        last_used = clust+1;	/* second of a pair */
    }
    index += 3;
  }

  printf("\n Cluster stats: %lu used, %lu bad, %lu items, %lu last.\n",
    countused, countbad, countitems, last_used);
  return ( cluststart +
    ( (1+last_used-2) * BPB_SECTORS_PER_CLUSTER(parameter_block.bpb) ) - 1 );
    /* we round up to the END of the cluster */
} /* BadClustPreserve12 */

