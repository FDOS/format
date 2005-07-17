/*
  I found this code on simtel, I think.  There was no copyright and no
  author's name on the source file.  So I am assuming it was placed
  under the public domain.  To protect this code, I am placing it
  under the GNU GPL.  -jhall
*/

/*
  **** NOTE!!!! ****

  This has code has been modified, by Brian E. Reifsnyder (BER), to support
  the syntax of the format command.
*/

/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to:
    The Free Software Foundation, Inc.,
    59 Temple Place - Suite 330,
    Boston, MA  02111-1307,
    USA.  */

/*
  The getopt() function parses the command line arguments.  Its
  arguments argc and argv are the argument count and array as passed
  to the main() function on program invocation.  An element of argv
  that starts with `/'is an option element.  If getopt() is called
  repeatedly, it returns successively each of the option characters
  from each of the option elements.

  If there are no more option characters, getopt() returns -1.  Then
  optind is the index in argv of the first argv element that is not an
  option.

  optstring is a string containing the legitimate option characters.
  If such a character is followed by a colon, the option requires an
  argument, so getopt places a pointer to the following text in the
  same argv-element, or the text of the following argv-element, in
  optarg.

  If getopt() does not recognize an option character, it prints an
  error message to stderr, stores the character in optopt, and returns
  `?'.  The calling program MUST TURN ON THIS BEHAVIOR by setting
  opterr to 1.
*/

/*
 * getopt - get option letter from argv
 */

#include <stdio.h>
#include <string.h>			/* for strcmp(), strchr() */


char    *optarg;        /* Global argument pointer. */
int     optind = 0;     /* Global argv index. */
int     opterr = 0;     /* Global error message flag. */

static char     *scan = NULL;   /* Private scan pointer. */

#if 0 /* disabled code */
extern char     *index();
#endif /* disabled code */


int
getopt(argc, argv, optstring)
int argc;
char *argv[];
char *optstring;
{
        register char c;
        register char *place;

	optarg = NULL;

	if (scan == NULL || *scan == '\0') {
		if (optind == 0)
			optind++;

		/*
		   The following line was modified by BER to support drive
		   letters, in the format command.
		*/
		if (optind >= argc || (argv[optind][0] != '/' && argv[optind][1] != ':') || argv[optind][1] == '\0')
			return(EOF);
		if (stricmp(argv[optind], "--")==0) {
		/* using stricmp because rest of FORMAT never uses strcmp */
			optind++;
			return(EOF);
		}

		scan = argv[optind]+1;
		optind++;
	}

	c = *scan++;
	place = strchr(optstring, c);

	if (place == NULL || c == ':') {
	  if (opterr) {
		printf("%s: unknown option /%c\n", argv[0], c);
		/* turn 2 only fprintf into printf - save overhead! */
		return('?');
	  }
	}

	/*
	   The following lines were added by BER to support drive
	   letters, in the format command.  If a '~' is returned, then
	   format.c will consider this opt to be the drive letter.
	*/

	if(c==':')
	  {
	  c='~';
	  optarg = argv[(optind-1)];
	  return(c);
	  }

	place++;
	if (*place == ':') {
		if (*scan != '\0') {
			optarg = scan;
			scan = NULL;
		} else if (optind < argc) {
			optarg = argv[optind];
			optind++;
		} else if (opterr) {
			printf("%s: /%c argument missing\n", argv[0], c);
			/* turn 2 only fprintf into printf - save overhead! */
			return('?');
		}
	}

	return(c);
}
