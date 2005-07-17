/*
File Name:  floppy.h
Description:  Floppy disk BPB table header file.
Author:  Brian E. Reifsnyder
Copyright: 2001 under the terms of the GNU GPL, Version 2
(Changed by Eric Auer 2003 - version 0.91c)
*/

#include "btstrct.h" /* STD_BPB */

#ifdef FLOP
  #define FEXTERN /* */
#else
  #define FEXTERN extern
#endif

void Set_Floppy_Media_Type(void);
int Format_Floppy_Cylinder(int,int);
void Compute_Sector_Skew(void);

#define HD 8 /* we need this in hdisk.c, drive_specs[HD] must be HD */
/* This is also used as a boundary between normal and extra formats!!! */

#ifdef FLOP
STD_BPB drive_specs[18]=
{
  /*
   BPS SEC/CLUST     SIDES (HEADS)
    v  v RES_SEC         SEC/CYL v HIDDEN
    v  v v FATS   SEC_PER_FAT  v v v v LARGE_CNT
    v  v v v  ROOT_DIR_ENT  v  v v v v v v
    v  v v v  v  SIZE DESC  v  v v v v v v
/* 5.25 inch 40 track: 0..3 (H/S: 1/8, 1/9, 2/8, 2/9) */
  {512,1,1,2, 64, 320,0xfe, 1, 8,1,0,0,0,0}, /* FD160  5.25 SS 40 */
  {512,1,1,2, 64, 360,0xfc, 2, 9,1,0,0,0,0}, /* FD180  5.25 SS 40 */
  {512,2,1,2,112, 640,0xff, 1, 8,2,0,0,0,0}, /* FD320  5.25 DS 40 */
  {512,2,1,2,112, 720,0xfd, 2, 9,2,0,0,0,0}, /* FD360  5.25 DS 40 */
/* 3.5 inch small:     4 */
  {512,2,1,2,112,1440,0xf9, 3, 9,2,0,0,0,0}, /* FD720  3.5  LD 80 */
/* 5.25 inch 80 track: 5 */
  {512,1,1,2,224,2400,0xf9, 7,15,2,0,0,0,0}, /* FD1200 5.25 HD 80 */
/* 3.5 inch big:       6 */
  {512,1,1,2,224,2880,0xf0, 9,18,2,0,0,0,0}, /* FD1440 3.5  HD 80 */
/* extra big:          7 */
  {512,2,1,2,240,5760,0xf0, 9,36,2,0,0,0,0}, /* FD2880 3.5  ED 80 */
/* HARDDISK:           8 */
  {512,0,1,2,512,   0,0xf8, 0, 0,0,0,0,0,0}, /* HD     HARD DISK */

 /* Non-Standard Format BPB Values (FAT sizes fixed in 0.91c) */
 /* Those all have skew 3, while the standard formats have skew 0 */
 /* They also have interleave 3 (DMF: 2) while standard have interleave 0 */

  /* DMF Format: variant of FD1680, used even by MS (and by IBM OS/2?) */
  {512,1,1,2,112,3360,0xf0,10,21,2,0,0,0,0}, /* FD1680 3.5  HD 80 */
  /* is this really 16 root dir entries? changing to 112! */

  /* More Sectors Only */
  {512,2,1,2,112, 800,0xfd, 2,10,2,0,0,0,0}, /* FD400  5.25 DS 40 */
  {512,2,1,2,112,1600,0xf9, 3,10,2,0,0,0,0}, /* FD800  3.5  LD 80 */
  {512,1,1,2,224,3360,0xf0,10,21,2,0,0,0,0}, /* FD1680x 3.5 HD 80 */
  {512,2,1,2,240,6720,0xf0,10,42,2,0,0,0,0}, /* FD3360 3.5  ED 80 */

  /* More Cylinders and Sectors */
  {512,1,1,2,224,2988,0xf9, 9,18,2,0,0,0,0}, /* FD1494 5.25 HD 83 */
  {512,1,1,2,224,3486,0xf0,11,21,2,0,0,0,0}, /* FD1743 3.5  HD 83 */

  {512,2,1,2,240,6972,0xf0,11,42,2,0,0,0,0}, /* FD3486 3.5  ED 83 */

  /* Suggestion: 81 and 82 cylinder formats!? 19 and 20 sector formats!? */

  /* END MARKER */
  {  0,0,0,0,  0,   0,0x00,0, 0,0,0,0,0,0}
};
#else
extern STD_BPB drive_specs[];
#endif


