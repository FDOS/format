/* Functions that emulate UNIX catgets */

/* Copyright (C) 1999,2000,2001 Jim Hall <jhall@freedos.org> */
/* Kitten version 2003 by Tom Ehlert, heavily modified by Eric Auer 2003 */

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

#include <stdio.h>			/* sprintf */
#include <stdlib.h>			/* getenv  */
#include <string.h>			/* strchr */
#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "kitten.h"

/* #define DEBUG_CATS 1 */
#ifdef DEBUG_CATS
#define dbgprintf(x) printf x
#else
#define dbgprintf(x)
#endif

/*
 * internal structure, must match between kitten.c and kittenc.c
 * Current definition matches kittenc version 2021-08-01
 */
 
/*
   beginning of catalog file (or executable)
   ---- (0 or more bytes) ----
   for each language {
     for each message number {
       message_header
       message
     }
     ---- (0 or more bytes) ----
   }
   ---- (0 or more bytes) ----
   for each language {
     content section
   }
   ---- (0 or more bytes) ----
   message_end
   end of catalog file (or executable)
*/

typedef signed short int16_t;
typedef signed long  int32_t;
typedef unsigned char uint8_t;
#define KITTEN_LANG_ID_LEN 4
#define KITTEN_MAGIC_LEN 8
#define KITTEN_MAGIC "KITTENC"  /* 8 characters including terminating \0 */

#define MAX_LANGUAGES_ATTACHED 100  /* determined by kittenc, but used to buffer this many content structures */

/* set# 0-255, msg# 0-255 */
#define MSG_ID(set, msg) (int16_t)((((int16_t)set) << 8) | ((int16_t)msg))

/* each message is preceeded by a message_header, messages are variable length */ 
struct message_header {
		int16_t len; /* strlen(message) + 1 for '\0' + sizeof(message_header) */
		int16_t id;  /* unique identifier of message to return   */
		/* strlen(message) has max size of 250 characters */
		/* uint8_t message[len-4]; */
};

/* stores location of all messages for a given language.
 * for all languages attached to a message catalog, a content section is created
 * and all content sections are stored as an array prior to message_end section
 * (with actual messages stored prior to content array)
 */
struct content {
		uint8_t language[KITTEN_LANG_ID_LEN];
		int32_t filepos_start;	/* for this language */
		int32_t filepos_end;
		int32_t reserved;		/* align on 16 byte  */
};

/* stored at the end of the file, used to calculate location of content sections */
struct message_end {
		int32_t filepos;               /* points to content array [1]*/
		int32_t resource_count;        /* number of content sections */
		int32_t fileend_orig;          /* file end NOW because UPX   */
		uint8_t id[KITTEN_MAGIC_LEN];  /* "KITTENC"                  */
		/* [1] filepos is based on a file of fileend_orig size, we therefore store
		 *     the original file size and calculate an adjustment based on actual
		 *     filesize when loading messages and add adjustment to filepos to
		 *     determine correct offset.  E.g. if file is compressed after appending
		 *     strings or some other manipulation causes filesize to change.
		 */
};

/*****/



/* Functions */

/**
 * On success, catgets() returns a pointer to an internal
 * buffer area containing the null-terminated message string.
 * On failure, catgets() returns the value 'message'.
 */

const char * catgets(nl_catd catalog, int setnum, int msgnum, const char *message)
{
	int16_t msg_id;
	struct message_header * p_message_header;
	
	/* validate catalog is open and initialized */
	if (catalog == NULL) return message;
	p_message_header = (struct message_header *)catalog;
	
	/* validate set and message # are within a possible range */
	if ((setnum > 255) || (msgnum > 255)) return message;
	msg_id = MSG_ID(setnum, msgnum);
	dbgprintf(("msg_id=%x (%x,%x)\n", msg_id, setnum, msgnum));
	
	/* cycle through all messages to find matching message */
	for ( ; p_message_header->len != 0; p_message_header = (struct message_header *)((char *)p_message_header + p_message_header->len))
	{
		if (p_message_header->id == msg_id)
		{
			/* on match, return string immediately after message_header; p_message_header->message */
			return (const char *)p_message_header + sizeof(struct message_header);
		}
	}
	
	/* if made it here, then no message matching the requested set & msg# was found */
	return message;
}


/**
 * Attempt to open message file and validate contents,
 * then load all messages for given language into memory.
 */
nl_catd catread(const char * const catfile, const char * const lang)
{
	int fd = -1;	/* file descriptor, aka file handle */
	int i;
	off_t seekoffset;
	int32_t seekadjustment;
	int32_t bufsize;
	uint8_t *buffer = NULL;
	struct message_end message_end;
	static struct content content[MAX_LANGUAGES_ATTACHED];
	
	/* try opening file */
	dbgprintf(("catread(%s,%s)\n", catfile, lang));
	if ((fd = open(catfile, O_RDONLY | O_BINARY)) < 0)
	{
		dbgprintf(("Unable to open file\n"));
		return NULL;  /* some error attempting to open file */
	}
	
	/* check end of file for our magic value */
	if ((seekoffset = lseek(fd, 0, SEEK_END)) < 0) 
		goto cleanup; /* error seeking */
	if (lseek(fd, - (int)sizeof(struct message_end), SEEK_END) < 0) 
		goto cleanup;  /* error seeking */
	if (read(fd, &message_end, sizeof(struct message_end)) != sizeof(struct message_end)) 
		goto cleanup; /* error reading */
	
	/* check for our MAGIC value */
	if (memcmp(message_end.id, "KITTENC", 8) != 0) 
		goto cleanup; /* not found, so cleanup and return failure */

	/* validate number of language sections within supported range */
	/* Note: 100 is max value kittenc will attach / supports */
	if (message_end.resource_count > MAX_LANGUAGES_ATTACHED)
		goto cleanup;
	
	/* calculate seek adjustment to account for catalog data relocated in file */
	seekadjustment = seekoffset - message_end.fileend_orig;
	
	/* load language list */
	if (lseek(fd, message_end.filepos + seekadjustment, SEEK_SET) != (message_end.filepos + seekadjustment)) 
		goto cleanup;  /* failed to seek */
	if (read(fd, &content, sizeof(struct content) * message_end.resource_count) != (off_t)(sizeof(struct content) * message_end.resource_count)) 
		goto cleanup; /* error reading or data corrupted */
	
	/* cycle through available languages and see if match to requested language */
	for (i = 0; i < message_end.resource_count; i++)
	{
		if ((lang[0] == toupper(content[i].language[0])) &&
		    (lang[1] == toupper(content[i].language[1])))
		{
			/* we found a match! so allocate memory for strings, read them in and return pointer to buffer */
			if (lseek(fd, content[i].filepos_start + seekadjustment, SEEK_SET) != (content[i].filepos_start + seekadjustment)) 
				goto cleanup; /* error seeking */
			bufsize = content[i].filepos_end - content[i].filepos_start;
			if ((buffer = (nl_catd)malloc(bufsize)) == NULL)
				goto cleanup; /* error allocating memory for messages */
			if (read(fd, buffer, bufsize) != bufsize)
			{
				/* error reading */
				free(buffer);
				goto cleanup;
			}
			/* successfully loaded messages, buffer points to first message_header */
			goto cleanup;
		}
	}
	/* if no match to language found then buffer is still NULL, so we fallback to internal strings */
	
	cleanup:
	dbgprintf(("catread() -- cleanup\n"));
	if (fd >= 0) close(fd);
	return (nl_catd)buffer;
}


/**
 * close/cleanup message catalog 
 */
void catclose(nl_catd catalog)
{
	if (catalog != NULL)
		free(catalog);
}


/**
 * Initialize kitten for program (name).
 */
nl_catd catopen(const char *name, int flag)
{
	char basename[16];			/* 8 character base filename */
	char catlang[4];			/* from LANG environment var. */
	char *nlsptr;				/* ptr to NLSPATH */
	char *lang;					/* ptr to LANG */
	char *s_ptr;				/* temp string pointer */

	nl_catd _kitten_catalog = NULL;
	static char catfile[260];	/* full path to _kitten_catalog */
	(void)flag;                 /* unused */

	dbgprintf(("catopen(%s)\n", name));

	/* We need value LANG to know which messages to display.  
	   If not specified, then we must default to internal messages.
	 */
	lang = getenv("LANG");	

	/* is LANG environment variable set? */
	if (lang == NULL) 
	{
		/* no, so use hardcoded internal strings */		
		dbgprintf(("LANG=NULL\n"));
		return NULL;
	}
	dbgprintf(("LANG=%s\n", lang));

	/* is LANG set to a valid 2 character language code? */
	memcpy(catlang, lang, sizeof(catlang));
	catlang[sizeof(catlang)-1] = '\0';
	if ( ( strlen(catlang) < 2 ) ||
	     ( (strlen(catlang) > 2) && (catlang[2] != '-') ) ) 
	{
		/* invalid formatted LANG value, so use internal strings */
		dbgprintf(("LANG format is invalid\n"));
		return NULL;
	}

	/* limit LANG used to just 2 character version,
	   ie. truncate in cases where -XX country code appended to language */
	catlang[2] = '\0';
	
	/* force uppercase */
	catlang[0] = (char)toupper(catlang[0]);
	catlang[1] = (char)toupper(catlang[1]);

  
	/* Open the _kitten_catalog file */


	/* first try name exactly as given,
	   this covers name is argv[0] or name given by explicit path
	*/
	_kitten_catalog = catread(name, catlang);
	
	/* otherwise try variation of name based on LANG */
	if (_kitten_catalog == NULL)
	{
		/* was explicit path given */
		if (strchr(name, '\\') != NULL)
		{
			/* if explicit path given, then don't try variations */
			/* return NULL; */
			
			/* strip path to get base filename */
			name = strrchr(name, '\\');
			if (name == NULL) return NULL;  /* this should never happen! */
			name = name + 1; /* skip past the final '\\' in name provided */
		}
		
		memcpy(basename, name, 8);
		basename[8] = '\0';
		/* strip extension if given */
		if ((s_ptr=strchr(basename, '.')) != NULL)
		{
			*s_ptr = '\0';
		}
		dbgprintf(("basename is %s\n", basename));

		/* try to locate the message _kitten_catalog using LANG & NLSPATH */

		/* step through NLSPATH */
		nlsptr = getenv ("NLSPATH");

		/* was NLSPATH found? */
		if (nlsptr == NULL) 
		{
			/* no, so default to current directory */
			nlsptr = ".";
		}

		catfile[0] = '\0';

		/* cycle through paths found in NLSPATH looking for catalog file */
		while ((nlsptr != NULL) && (_kitten_catalog == NULL)) 
		{
			char *tok = strchr(nlsptr, ';');
			int toklen;

			if (tok == NULL)
				toklen = strlen(nlsptr); /* last segment */
			else
				toklen = (int)(tok - nlsptr); /* segment terminated by ';' */
      
			/* catfile = malloc(toklen+1+strlen(name)+1+strlen(lang)+1); */
			/* Try to find the _kitten_catalog file in each path from NLSPATH */

			/* check for potential overflow in NLSPATH, and skip this segment */
			if ((toklen+6+strlen(name)) > sizeof(catfile)) 
				continue;

			memcpy(catfile, nlsptr, toklen);
			if ((toklen > 0) && (catfile[toklen-1] == '\\'))
			{
				/* nlspath segment ends in directory separator */
				catfile[toklen] = '\0';
			} else {
				/* we need to append directory separator */
				strcpy(catfile+toklen,"\\");
				toklen++;
			}

			/* Rule #1: %NLSPATH%\%LANG%\basename */
			strcat(catfile,catlang);
			strcat(catfile,"\\");
			strcat(catfile,basename);
			_kitten_catalog = catread(catfile, catlang);
			if (_kitten_catalog != NULL) 
				continue;

			/* Rule #2: %NLSPATH%\basename.%LANG% */
			catfile[toklen] = '\0';  /* truncate catfile back to "%NLSPATH%\" portion */
			strcat(catfile,basename);
			strcat(catfile,".");
			strcat(catfile,catlang);
			_kitten_catalog = catread(catfile, catlang);
			if (_kitten_catalog != NULL)
				continue;


			/* Grab next tok for the next while iteration */
			nlsptr = tok;
			if (nlsptr != NULL) nlsptr++;
      
		} /* while tok */

		if (_kitten_catalog == NULL)
		{
			/* We could not find a valid message catalog.  Return failure to use internal strings. */
			dbgprintf(("No valid message catalog found!\n"));
			return (NULL);
		}
	}
	
	/* catalog is opened, return our handle to message set buffer */
	dbgprintf(("CATGETS catalog loaded!\n"));
	return _kitten_catalog;
}