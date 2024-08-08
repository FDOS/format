/*
// Program:  Format
// Version:  0.91t
// (0.90b/c - compiler warnings fixed - Eric Auer 2003)
// (0.91i - parameter_block.bpb, not drive_specs... - Eric Auer 2003)
// (0.91j - FAT32 record bad cluster list support - Eric Auer 2003)
// (0.91k... - Eric Auer 2004)
// (0.91t - message tuning - Eric Auer 2005)
// Written By:  Brian E. Reifsnyder
// Copyright:  2002-2005 under the terms of the GNU GPL, Version 2
// Module Name:  recordbc.c
// Module Description:  Bad Sector Recording Functions
*/

#include "format.h"
#include "floppy.h"
#include "driveio.h"
#include "btstrct.h"


void Record_Bad_Clusters_FAT12(void);
void Record_Bad_Clusters_FAT16(void);
void Record_Bad_Clusters_FAT32(void);


void unusable_wimp(void);
void unusable_wimp(void) /* give up format attempt, broken metadata sector */
{
    printf("Unusable disk: Bad sector in system data. Format terminated.\n");
    Exit(4,30);
} /* unusable_wimp */


int Is_Even(unsigned long number)
{
  if( (number&0x01)>0 ) return FALSE;

  return(TRUE);
}

void Record_Bad_Clusters(void)
{
  if (debug_prog==TRUE) printf("[DEBUG]  Record Bad Clusters ");
  if (param.fat_type==FAT12) Record_Bad_Clusters_FAT12();
  if (param.fat_type==FAT16) Record_Bad_Clusters_FAT16();
  if (param.fat_type==FAT32) Record_Bad_Clusters_FAT32();
}

void Record_Bad_Clusters_FAT12(void)
{
  unsigned long bad_cluster;
  unsigned long last_bad_cluster;
  unsigned char fat12_fat[6145];

  int index;
  int sector;

  unsigned long first_data_sector;
  unsigned long first_root_sector;
  
  if (debug_prog==TRUE) printf("FAT12\n");

  first_data_sector = parameter_block.bpb.reserved_sectors;
  first_data_sector += parameter_block.bpb.number_of_fats
    * parameter_block.bpb.sectors_per_fat;
  first_root_sector = first_data_sector;
  first_data_sector += 32UL * parameter_block.bpb.root_directory_entries
    / parameter_block.bpb.bytes_per_sector;

  /* Is the first bad sector in the boot sector, fat tables, or directory? */
  if (bad_sector_map[0] < first_data_sector)
    {
    /* Terminate the program... the disk is not usable. */

    if (bad_sector_map[0] < parameter_block.bpb.reserved_sectors)
      {
      printf("\nBoot sector broken.\n");
      }
    else
      {
      if (bad_sector_map[0] < first_root_sector)
        printf("\nFAT sector broken.\n");
      else
        printf("\nRoot directory sector broken.\n");
      }

    unusable_wimp();	/* can happen for all FAT types - common code (0.91p) */

    } /* fatal broken sector */

  /* Clear the FAT12 FAT table buffer. */
  for (index = 0; index < sizeof(fat12_fat); index++) /* 6144 bytes */
    fat12_fat[index]=0; /* off-by-1 fixed in 0.91i */

  /* Create initial entries. */
  fat12_fat[0] = parameter_block.bpb.media_descriptor;
  fat12_fat[1] = 0xff;
  fat12_fat[2] = 0xff;

  /* Now, translate the bad sector map into the FAT12 table. */

  last_bad_cluster = 0xffff;
  drive_statistics.allocation_units_with_bad_sectors = 0; /* 0.91c */

  index=0;
  do
    {
    /* Translate the sector into a cluster number. */
    /* *** is this correct? *** */
    bad_cluster = ( (bad_sector_map[index] - first_data_sector)
     + BPB_SECTORS_PER_CLUSTER(parameter_block.bpb) - 1 ) /* round up */
       / BPB_SECTORS_PER_CLUSTER(parameter_block.bpb);
    bad_cluster += 2; /* 2 based counting */

    if (bad_cluster != last_bad_cluster)
      {
      last_bad_cluster = bad_cluster;
      drive_statistics.allocation_units_with_bad_sectors++;
      }

    /* Determine if the cluster is even or odd and record the cluster  */
    /* in the FAT table buffer.                                        */
    if(TRUE==Is_Even(bad_cluster) ) /* even cluster number: */
      {
      fat12_fat[(unsigned)((bad_cluster*3/2) +0 )] = 0xf7;
        /* LOW 2 nibbleS */
      fat12_fat[(unsigned)((bad_cluster*3/2) +1 )] = 0x0f
        | (fat12_fat[(unsigned)((bad_cluster*3/2) +1 )] & 0xf0);
        /* HIGH nibble inserted in LOW half of byte */
      /* EVEN: triple[0] = lowbyte, triple[1].lownibble = highnibble */
      /* (bad_cluster*3/2) points to start of a triple +0 */
      }
    else /* odd cluster number: */
      {
      fat12_fat[(unsigned)((bad_cluster*3/2) +(1-1) )] = 0x70
        | (fat12_fat[(unsigned)((bad_cluster*3/2) +(1-1) )] & 0x0f);
        /* LOW nibble inserted in HIGH half of byte */
      fat12_fat[(unsigned)((bad_cluster*3/2) +(2-1) )] = 0xff;
        /* HIGH 2 nibbleS */
      /* ODD: triple[1].highnibble = lownibble, triple[2] = highbyte */
      /* (bad_cluster*3/2) points to start of a triple +1 !!! */
      /* --> caused wrong FAT12 entries in versions before 0.91c */
      }

     /* FAT12 packs 0x0123 0x0abc into 0x23 0xc1 0xab */

    index++;
    } while (bad_sector_map[index]!=0);

  /* Finally, write the fat table buffer to the disk...twice. */
  for (sector = 0; sector < parameter_block.bpb.sectors_per_fat; sector++)
    {
    unsigned int fat_index = sector * 512;

    for (index = 0; index < 512; index++)
      sector_buffer[index] = fat12_fat[fat_index + index];

    if (debug_prog==TRUE)
      printf("[DEBUG]  Recording FAT Sector->  %3d\n",sector);

    Drive_IO(WRITE, sector + parameter_block.bpb.reserved_sectors, 1);
    if (parameter_block.bpb.number_of_fats == 2)
      Drive_IO(WRITE, (sector + parameter_block.bpb.reserved_sectors
        + parameter_block.bpb.sectors_per_fat), 1);

    } /* for each FAT sector */
}

void Record_Bad_Clusters_FAT16(void)
{
  unsigned bad_sector_map_index;
  unsigned long current_fat_sector;
  int dirty_sector_buffer;

  unsigned long cluster_range_low;  /* "long" not really needed */
  unsigned long cluster_range_high; /* ditto */

  unsigned long bad_cluster;
  unsigned long last_bad_cluster;
  /* unsigned long sector_number_of_sector_buffer; */

  unsigned long first_data_sector;
  unsigned long first_root_sector;

  if (debug_prog==TRUE) printf("FAT16\n");

  first_data_sector = parameter_block.bpb.reserved_sectors;
  first_data_sector += parameter_block.bpb.number_of_fats
    * parameter_block.bpb.sectors_per_fat;
  first_root_sector = first_data_sector;
  first_data_sector += 32UL * parameter_block.bpb.root_directory_entries
    / parameter_block.bpb.bytes_per_sector;

  /* Is the first bad sector in the boot sector, fat tables, or directory? */
  if (bad_sector_map[0] < first_data_sector)
    {
    /* Terminate the program... the disk is not usable. */

    if (bad_sector_map[0] < parameter_block.bpb.reserved_sectors)
      {
      printf("\nBoot sector broken.\n");
      }
    else
      {
      if (bad_sector_map[0] < first_root_sector)
        printf("\nFAT sector broken.\n");
      else
        printf("\nRoot directory sector broken.\n");
      }

    unusable_wimp();	/* can happen for all FAT types - common code (0.91p) */

    } /* fatal broken sector */

  bad_cluster = 0;
  last_bad_cluster = 0xffff;

  Clear_Sector_Buffer();

  bad_sector_map_index = 0;
  dirty_sector_buffer = FALSE;
  current_fat_sector = parameter_block.bpb.reserved_sectors;
  do
    {
    cluster_range_low  = ((current_fat_sector - parameter_block.bpb.reserved_sectors) << 8);
    cluster_range_high = cluster_range_low + 255; /* 512by/sect -> 256 slots/sect */

    do
      {
      if (bad_sector_map[bad_sector_map_index] != 0)
	{
	/* *** changed in 0.91i - hope it is correct now??? *** */
	bad_cluster = (
	   (bad_sector_map[bad_sector_map_index] - first_data_sector)
	   + BPB_SECTORS_PER_CLUSTER(parameter_block.bpb) - 1 ) /* round up */
	 / BPB_SECTORS_PER_CLUSTER(parameter_block.bpb);
	bad_cluster += 2; /* 2 based counting */
	}
      else
        bad_cluster=100000L; /* Set to dummy value. */

      if ( (bad_cluster != last_bad_cluster) && (bad_cluster != 100000L) )
	{
	drive_statistics.bad_sectors +=
	  BPB_SECTORS_PER_CLUSTER(parameter_block.bpb);

	last_bad_cluster = bad_cluster;
	drive_statistics.allocation_units_with_bad_sectors++; /* 0.91c */
	}

      if (  (bad_cluster >= cluster_range_low)
         && (bad_cluster <= cluster_range_high) )
	{
	sector_buffer[(unsigned)( (2 * (bad_cluster & 0xff) )     )] = 0xf7;
	sector_buffer[(unsigned)( (2 * (bad_cluster & 0xff) ) + 1 )] = 0xff;

	dirty_sector_buffer=TRUE;
	bad_sector_map_index++;
	}

      } while (  (bad_cluster <= cluster_range_high)
              && (bad_sector_map[bad_sector_map_index] != 0) );
        /* work on the same FAT sector as long as possible */

    if (dirty_sector_buffer==TRUE) /* if anything inside, write it */
      {
      if (current_fat_sector==1)
	{
	/* Create initial entries. */
	sector_buffer[0]=parameter_block.bpb.media_descriptor;
	sector_buffer[1]=0xff;
	sector_buffer[2]=0xff;
	sector_buffer[3]=0xff;
	}

      Drive_IO(WRITE, current_fat_sector, 1);
      if (parameter_block.bpb.number_of_fats == 2)
        Drive_IO(WRITE,
          (current_fat_sector + parameter_block.bpb.sectors_per_fat), 1);

      Clear_Sector_Buffer();
      dirty_sector_buffer=FALSE;
      }

    current_fat_sector++;
    } while (current_fat_sector <= parameter_block.bpb.sectors_per_fat);
}

void Record_Bad_Clusters_FAT32(void) /* added actual FAT writing in 0.91j */
{
  unsigned bad_sector_map_index = 0;
  unsigned long bad_cluster;
  unsigned long fat_size_32;
  unsigned long first_data_sector;
  unsigned long first_fat_sector = parameter_block.bpb.reserved_sectors;

  if (debug_prog==TRUE) printf("FAT32\n");

  fat_size_32 = parameter_block.xbpb.fat_size_high;
  fat_size_32 <<= 16;
  fat_size_32 |= parameter_block.xbpb.fat_size_low;

  /* Note that this is a DIFFERENT formula for FAT32 and FAT12/FAT16 ! */
  first_data_sector = first_fat_sector;
  first_data_sector += parameter_block.bpb.number_of_fats * fat_size_32;

  /* after this there are some CLUSTERS used by the root directory, but   */
  /* we do not try to relocate the root directory if it has broken parts! */

  if (bad_sector_map[0] < first_data_sector)
    {
    /* Terminate the program... the disk is not usable. */

    if (bad_sector_map[0] < parameter_block.bpb.reserved_sectors)
      printf("\nReserved sector (boot, info, backup of either...) broken.\n");
    else
      printf("\nFAT sector broken.\n");

    unusable_wimp();	/* can happen for all FAT types - common code (0.91p) */

    }

  while ( (bad_sector_map[bad_sector_map_index] != 0) )
    {
    unsigned long fat_sector;
    unsigned long fat_value;
    unsigned fat_index;

    /* *** changed in 0.91i - correct now? *** */
    bad_cluster = (
      ( bad_sector_map[bad_sector_map_index] - first_data_sector )
      + BPB_SECTORS_PER_CLUSTER(parameter_block.bpb) - 1 ) /* round up */
      / BPB_SECTORS_PER_CLUSTER(parameter_block.bpb);
    bad_cluster += 2; /* 2 based counting */

    /* no extra rounding needed... 4 bytes per FAT entry. */
    fat_sector = first_fat_sector;
    fat_sector += bad_cluster / (512 >> 2);
    fat_index = ((unsigned)bad_cluster) & ( (512 >> 2) - 1);
    fat_index <<= 2;

    Drive_IO(READ, fat_sector, 1);

    fat_value = sector_buffer[fat_index+3];
    fat_value <<= 8;
    fat_value |= sector_buffer[fat_index+2];
    fat_value <<= 8;
    fat_value |= sector_buffer[fat_index+1];
    fat_value <<= 8;
    fat_value |= sector_buffer[fat_index+0];

    if (fat_value == 0)
      printf("*** Bad sector %lu, cluster %lu marked bad in FAT! ***\n",
        bad_sector_map[bad_sector_map_index], bad_cluster);

    if ( (fat_value < 0x0ffffff0UL) || (fat_value > 0x0ffffff7UL) ) /* not yet marked? */
      {
      if (fat_value != 0)
        printf("WARNING: Have to mark USED cluster %lu as bad!\n", bad_cluster);

      drive_statistics.allocation_units_with_bad_sectors++;

      sector_buffer[fat_index+0] = 0xf0; /* mark cluster as bad */
      sector_buffer[fat_index+1] = 0xff;
      sector_buffer[fat_index+2] = 0xff; /* range for the "bad" mark: */
      sector_buffer[fat_index+3] = 0x0f; /* 0x0ffffff0 ... 0x0ffffff7 */

      Drive_IO(WRITE, fat_sector, 1);
      if (parameter_block.bpb.number_of_fats == 2)
        Drive_IO(WRITE, (fat_sector + fat_size_32), 1);

      } /* had to update FAT */

    bad_sector_map_index++;
    } /* loop over bad sector list */

  printf("*** Found %u bad sectors. Marked %lu clusters as bad. ***\n",
    bad_sector_map_index, drive_statistics.allocation_units_with_bad_sectors);
  
}

