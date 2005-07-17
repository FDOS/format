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
This is part of getopt.c, which is under the GNU GPL.  Please see the
getopt.c file for details.

*/


#ifndef _GETOPT_H
#define _GETOPT_H

#ifdef __cplusplus
extern "C" {
#endif

extern char    *optarg;        /* Global argument pointer. */
extern int     optind;         /* Global argv index. */

extern int     opterr;         /* Global error message flag. */

int getopt(int argc, char *argv[], char *optstring);

#ifdef __cplusplus
}
#endif

#endif
