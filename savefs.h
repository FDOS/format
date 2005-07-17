/*
// Program:  Format
// Written By:  Brian E. Reifsnyder
// Version:  0.91r (update 0.91l -> 0.91r: more arguments -ea)
// Copyright:  2002-2004 under the terms of the GNU GPL, Version 2
*/

#ifndef SAVEFS_H
#define SAVEFS_H 1

extern void Save_File_System(int overwrite);	/* 0.91r: overwrite */
extern void Restore_File_System(void);	/* 0.91l by Eric Auer 2004 */

extern unsigned long BadClustPreserve(void);	/* 0.91k by Eric Auer 2004 */
/* BadClustPreserve is in bcread.c to keep savefs.c smaller...     */
/* 0.91r: returns sector number of last sector of last filled cluster */

#endif
