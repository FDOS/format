/*
// Program:  Format
// Version:  0.91k
// (0.90b/c/d - DMA friendly buffer movement - Eric Auer - May 2003)
// (0.91k ... Eric Auer 2004)
// Written By:  Brian E. Reifsnyder
// Copyright:  2002-2004 under the terms of the GNU GPL, Version 2
// Module Name:  init.c
// Module Description:  Program initialization functions.
*/

#include <stdlib.h>

#include "format.h"
#include "driveio.h" /* sector buffers */

void Setup_DDPT(void);


void Initialization(void)
{
  int bad_sector_map_pointer;
  unsigned long int where;

  bad_sector_map_pointer = 0;
  sector_buffer = &sector_buffer_0[0];
  where = FP_SEG(sector_buffer);
  where <<= 4;
  where += FP_OFF(sector_buffer);
  if ((where & 0xffff) > (0xffff - 512)) {
    sector_buffer = &sector_buffer_1[0];
    where = FP_SEG(sector_buffer);
    where <<= 4;
    where += FP_OFF(sector_buffer);
    printf("DMA tuning: using alternative sector_buffer\n");
  }
  if ((where & 0xffff) > (0xffff - 512)) {
    printf("DMA tuning: CANNOT move sector buffer away from boundary!?\n");
  }

  huge_sector_buffer = &huge_sector_buffer_0[0];
  where = FP_SEG(huge_sector_buffer);
  where <<= 4;
  where += FP_OFF(huge_sector_buffer);
  if ( (where & 0xffff) > (0xffff - sizeof(huge_sector_buffer_0)) ) {
    huge_sector_buffer = &huge_sector_buffer_1[0];
    where = FP_SEG(sector_buffer);
    where <<= 4;
    where += FP_OFF(sector_buffer);
    printf("DMA tuning: using alternative huge_sector_buffer\n");
  }

  param.drive_letter[0]=NULL;
  param.volume_label[0]=NULL;

  param.drive_type=0; /* not NULL; */
  param.drive_number=0; /* not NULL; */
  param.fat_type=0; /* not NULL; */
  param.media_type=UNKNOWN;

  param.force_yes=FALSE;
  param.verify=TRUE;

  param.v=FALSE;
  param.q=FALSE;
  param.u=FALSE;
  param.f=FALSE;
  param.b=FALSE;
  param.s=FALSE;
  param.t=FALSE;
  param.n=FALSE;
  param.one=FALSE;
  param.four=FALSE;
  param.eight=FALSE;

  param.size=UNKNOWN;
  param.cylinders=0;
  param.sectors=0;

  drive_statistics.bad_sectors = 0;
  drive_statistics.bytes_per_sector = 512;

  segread(&sregs);

  /* Clear bad_sector_map[]. */
  do
    {
    bad_sector_map[bad_sector_map_pointer]=0;
    bad_sector_map_pointer++;
    } while (bad_sector_map_pointer<MAX_BAD_SECTORS);

  Setup_DDPT();
}

void Setup_DDPT(void)
{
  /* Get the location of the DDPT */

    regs.h.ah =0x35;
    regs.h.al =0x1e;
    intdosx(&regs, &regs, &sregs);

    ddpt = MK_FP(sregs.es, regs.x.bx);
}

