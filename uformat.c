/*
// Program:  Format
// Version:  0.91v
// (0.90b/c/d fixing compiler warnings - Eric Auer 2003)
// (0.91b..i all kinds of fixes and clean-ups - Eric Auer 2003)
// (0.91k... Eric Auer 2004 (no updates in o/p/q))
// (0.91t - some message tuning, conio removed - Eric Auer 2005)
// (0.91v - small "error in track 0" optimization - Eric Auer 2006)
// Written By:  Brian E. Reifsnyder
// Copyright:  2002-2006 under the terms of the GNU GPL, Version 2
// Module Name:  uformat.c
// Module Description:  Unconditional Format Functions
*/

#define UFORMAT

#include "format.h"
#include "uformat.h"
#include "floppy.h"
#include "driveio.h"
#include "userint.h"

#if 0		/* conio involves annoying overhead: VPRINTER, SCROLL... */
// #include <conio.h> /* kbhit, getch, which use int 21.7 and 21.b or similar */
#endif		/* removing conio and adding prf.c saves 2.4k UPXed size */

int my_kbhit(void);
int my_getch(void);

void Unconditional_Floppy_Format(void);
void Unconditional_Hard_Disk_Format(void);
void Compute_Sector_Skew(void);

int my_kbhit()
{
  regs.x.ax = 0x0b00;	/* get stdin status */
  intdos(&regs, &regs);
  return regs.h.al;	/* nonzero if char available */
}

int my_getch()
{
  regs.x.ax = 0x0700;	/* char input without echo */
  intdos(&regs, &regs);
  return regs.h.al;	/* ASCII value */
}

/* Unconditionally Format the Drive */
void Unconditional_Format()
{
  bad_sector_map_pointer=0;

  if (param.drive_type==FLOPPY) {
    Unconditional_Floppy_Format();
  } else {
    Unconditional_Hard_Disk_Format();
  }
}

void Unconditional_Floppy_Format()
{
  int index = 0;
  int buggy = 0;
  unsigned long percentage;
  int drive_number;

  int cylinders = parameter_block.bpb.total_sectors;
  cylinders /= parameter_block.bpb.number_of_heads;
  cylinders /= parameter_block.bpb.sectors_per_cylinder;

  /* Reset the floppy disk controller */
  drive_number = param.drive_number;
  regs.h.ah = 0x00;
  regs.h.dl = drive_number;
  int86( 0x13, &regs, &regs); /* int 13.0 - rereads DDPT, resets controller */

  /* Set huge_sector_buffer to all 0xf6's. */
  memset(huge_sector_buffer_0, 0xf6, sizeof(huge_sector_buffer_0));
  memset(huge_sector_buffer_1, 0xf6, sizeof(huge_sector_buffer_1));
  /* both! 0.91d */

  do
    {
    /* is it correct that ONE SIDED means "head 0 only" ? */
    buggy += Format_Floppy_Cylinder(index, 0);

    if ((index==0) && (buggy>0)) /* -ea */
      {
      printf("Format error in track 0, giving up.\n");
      Exit(4,35);
      }

    if (parameter_block.bpb.number_of_heads==2)
      buggy += Format_Floppy_Cylinder(index, 1);

    percentage = (100*index) / cylinders;

    Display_Percentage_Formatted(percentage);

    Compute_Sector_Skew();

    index++;
    } while (index < cylinders);
    /* must be < not <= (formatted one track too much in 0.91 ! */

    if (percentage != 100) Display_Percentage_Formatted(100);

    if (buggy>0)
      printf("\nFound %d bad sectors during formatting.\n", buggy);
    else
      printf("\n");
}



/* Format harddisk, do a surface scan / wipe. Always 512 bytes per sector! */
void Unconditional_Hard_Disk_Format()
{	/* Bugfix 0.91k: process I/O errors properly in surface scan. */
  int number_of_sectors;
  char correct_sector[512]; /* new in 0.91g, faster */

  unsigned long percentage_old = 999;
  unsigned long mbytes_old = 999;	/* 0.91v */

  unsigned index = 0;

  unsigned long last_bad_sector;
  unsigned long sector_number;
  unsigned long percentage;
  unsigned long mbytes;			/* 0.91v */

  unsigned long max_logical_sector = parameter_block.bpb.total_sectors; /* assume 16bit first */
   
  unsigned long badsec1 = 0; /* -ea */
  unsigned long badsec2 = 0; /* -ea */

  if (max_logical_sector == 0) { /* if 16bit value is 0, use 32bit value */
     /* typo fixed in here 0.91g+ ... had superfluous IF here ... */
     max_logical_sector = parameter_block.bpb.large_sector_count_high;
     max_logical_sector <<= 16;
     max_logical_sector |= parameter_block.bpb.large_sector_count_low;
  }

  number_of_sectors = sizeof(huge_sector_buffer_0)>>9; /* -ea */
  if (number_of_sectors < 1) {
    printf("internal buffer error!\n");
    number_of_sectors = 1; /* use small sector_buffer */
  }

  bad_sector_map_pointer = 0;
  last_bad_sector = 0xffffffffUL;
  sector_number = 1; /* start after boot sector */

  for (index = 0; index < MAX_BAD_SECTORS; index++)
    bad_sector_map[index] = 0;

  memset(&correct_sector[0], 0xf6, 512); /* a nice and shiny empty sector */

  /* Clear and check for bad sectors (maximum of 1 buffer full at a time) */
  printf(" Zapping / checking %lu sectors\n", max_logical_sector); /* -ea */
  if (max_logical_sector > 0x11000UL) /* 0.91g+: 34 MB. Enough for all FATs. */
    {
    printf("To skip the rest of the surface scan after the first 34 MB\n");
    printf("AT OWN RISK, press ESC (only checked at 'percent boundaries').\n");
    }

  do
    {
    int surface_error;

    surface_error = 0;

    /* clear small buffer (used if number_of_sectors == 1) */
    memcpy((void *)&sector_buffer[0], &correct_sector[0], 512);
    
    /* also clear big buffer (used in all other cases) */
    for (index=0; index < number_of_sectors; index++) {
      memcpy((void *)&huge_sector_buffer[index<<9], &correct_sector[0], 512);
    }
    
    if ((sector_number + number_of_sectors) > max_logical_sector) {
      int howmany; /* clip last chunk -ea */
      howmany = (int)(max_logical_sector - sector_number);
      if ( howmany > number_of_sectors ) howmany = number_of_sectors;
      while ( (number_of_sectors > 1) && (howmany < 2) )
        { /* make sure that huge_sector_buffer is used!  */
          sector_number--; /* the trick is to overlap... */
          howmany++;       /* ...with the previous chunk */
        }
      surface_error |= Drive_IO(WRITE, sector_number, -howmany);
      surface_error |= Drive_IO(READ,  sector_number, -howmany);
    } else {
      surface_error |= Drive_IO(WRITE, sector_number, -number_of_sectors);
      surface_error |= Drive_IO(READ,  sector_number, -number_of_sectors);
    }

    /* Check for bad sectors by comparing the results of the sector read. */
    /* Changed in 0.91g to use memcmp, not checking every byte manually.  */
    index = 0;
    do
      {
      int cmpresult = 0;
      if (surface_error == 0) {
        if (number_of_sectors == 1) {
          cmpresult = memcmp((void *)&sector_buffer[0], &correct_sector[0], 512);
        } else {
          cmpresult = memcmp((void *)&huge_sector_buffer[index<<9], &correct_sector[0], 512);
        }
      } /* else: I/O error reported, assume ALL sectors were wrong. */
      if ( cmpresult || surface_error )
	{

        if ( ( sector_number + index ) < 5 ) /* new 0.91d */
          {
          printf("One of the first 5 sectors is broken. FORMAT not possible.\n");
          Exit(4,36);
          }

	if ( last_bad_sector != ( sector_number + index ) )
	  {
	  bad_sector_map[bad_sector_map_pointer]
	    = sector_number + index;

	  bad_sector_map_pointer++; /* fixed in 0.91c */
	  if (bad_sector_map_pointer >= MAX_BAD_SECTORS) /* new in 0.91c */
	    {
	    printf("Too many bad sectors! FAT bad sector map will be incomplete!\n");
	    bad_sector_map_pointer--;
	    } /* too many bad sectors */

	  badsec1++; /* -ea */
	  } /* new bad sector */

	} /* any bad sector */

      index++;
      } while (index < number_of_sectors ); /* one buffer full */
      /* size of buffer was assumed to be 32*512 (wrong!) 0.91d */

    percentage = (max_logical_sector < 65536UL)      ?
      ( (100 * sector_number) / max_logical_sector ) :
      ( sector_number / (max_logical_sector/100UL) ) ;
      /* improved in 0.91g+ */

    mbytes = sector_number / (1024L*1024/512);	/* 0.91v */

    if (mbytes!=mbytes_old)	/* 0.91v: more frequent update for large disks */
      {
      mbytes_old = mbytes;
      printf("%lu MBytes ", mbytes);	/* 0.91v */
      Display_Percentage_Formatted(percentage);
      }

    if (percentage!=percentage_old)
      {
      percentage_old = percentage;
      printf("%lu MBytes ", mbytes);	/* 0.91v */
      Display_Percentage_Formatted(percentage);
      if (badsec1 != 0)
        { /* added better output -ea */
        printf("\n [errors found]\n");
        badsec2 += badsec1;
        badsec1 = 0;
        }
      if ( (sector_number > 0x11000UL) && my_kbhit() && (my_getch() == 27) )
        {	/* surface scan / wipe abort possibility added 0.91g+ */
        printf("\nESC pressed - skipping surface scan!\n");
        sector_number = max_logical_sector;
        }
      }

    sector_number += number_of_sectors; /* 32; fixed in 0.91g+ */
    
    } while (sector_number<max_logical_sector);

    if (percentage != 100) Display_Percentage_Formatted(100);
    
    badsec2 += badsec1;
    if (badsec2>0) {
      printf("\n %lu errors found.\n", badsec2);
    } else {
      printf("\n No errors found.\n");
    }

}
