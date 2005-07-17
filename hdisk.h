/*
// Program:  Format
// Version:  0.91m
// Written By:  Brian E. Reifsnyder
// Copyright:  2002-2004 under the terms of the GNU GPL, Version 2
*/

#ifndef HDISK_H
#define HDISK_H 1

#ifdef HDISK
  #define HDISKEXTERN    /* */
#else
  #define HDISKEXTERN  extern
#endif



HDISKEXTERN void Get_Device_Parameters(void);


HDISKEXTERN void Force_Drive_Recheck(void); /* formerly called Set_DPB_Access_Flag() */
HDISKEXTERN void Set_Hard_Drive_Media_Parameters(int alignment); /* align added 0.91m */

#endif

