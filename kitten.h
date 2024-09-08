/* Functions that emulate UNIX catgets, some small DOS file functions */

/* Copyright (C) 1999,2000 Jim Hall <jhall@freedos.org> */
/* Kitten version by Tom Ehlert, heavily modified by Eric Auer 2003 */
/* Updated 2024 to use kittenc internal format */

/*
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef _CATGETS_H
#define _CATGETS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Data types */
typedef void * nl_catd;

/* Functions */
#ifdef NOCATS             /* #define NOCATS to disable completely */
#define catgets(c,x,y,s) s
#define catopen(x,y) 1
#define catclose(x)
#else
/* return translated message, if not found or error returns message
 * in catalog, supports up to 255 sets of 255 messages
*/
char const * catgets(nl_catd catalog, int setnum, int msgnum, char const * const message);

/* open translation catalog and return handle - only 1 catalog file supported 
 * generally will pass argv[0], i.e. executable file, but can pass other filenames
 * Note: unlike older FreeDOS catgets & kitten, these message catalogs are binary not text
 */
nl_catd catopen(const char *name, int flag);

/* release any resources */
void catclose(nl_catd catalog);
#endif


#ifdef __cplusplus
}
#endif

#endif /* _CATGETS_H */
