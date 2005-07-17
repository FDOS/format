/*
// Program:  Format
// Version:  0.91r
// (0.90b/c - DMA boundary avoidance by Eric Auer - May 2003)
// (0.91r: added Exit() - Eric 2004)
// Written By:  Brian E. Reifsnyder
// Copyright:  2002 under the terms of the GNU GPL, Version 2
*/

#ifndef DRIVEIO_H
#define DRIVEIO_H

#ifdef DIO
  #define DIOEXTERN   /* */
#else
  #define DIOEXTERN   extern
#endif


typedef unsigned long ULONG;
typedef unsigned long  __uint32;
typedef unsigned short __uint16;

/* Buffers */
DIOEXTERN unsigned char sector_buffer_0[512];
DIOEXTERN unsigned char sector_buffer_1[512];
DIOEXTERN unsigned char huge_sector_buffer_0[(512*21)]; /* old: 512*32 */
DIOEXTERN unsigned char huge_sector_buffer_1[(512*21)]; /* old: 512*32 */
/* only uformat.c uses huge_sector_buffer at all (for fill with 0xf6), */
/* so huge_sector_buffer does NOT have to be one track big!            */

#ifdef DIO
DIOEXTERN unsigned char far * sector_buffer = &sector_buffer_0[0];
DIOEXTERN unsigned char far * huge_sector_buffer = &huge_sector_buffer_0[0];
#else
DIOEXTERN unsigned char far * sector_buffer;
DIOEXTERN unsigned char far * huge_sector_buffer;
#endif

/* The following lines were taken from partsize.c, written by Tom Ehlert. */

DIOEXTERN void Clear_Huge_Sector_Buffer(void);
DIOEXTERN void Clear_Sector_Buffer(void);

DIOEXTERN void Exit(int mserror, int fderror);	/* by Eric Auer for 0.91r */
DIOEXTERN void Lock_Unlock_Drive(int lock);	/* by Eric Auer for 0.91l */

DIOEXTERN int Drive_IO(int command,unsigned long sector_number,int number_of_sectors);
DIOEXTERN void Enable_Disk_Access(void);

DIOEXTERN unsigned int FAT32_AbsReadWrite(char DosDrive, int count, ULONG sector, void far *buffer, unsigned ReadOrWrite);
DIOEXTERN unsigned int TE_AbsReadWrite(char DosDrive, int count, ULONG sector, void far *buffer, unsigned ReadOrWrite);

#endif
