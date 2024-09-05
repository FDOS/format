/*
// Program:  Format
// Version:  0.91v
// (0.90b/c/d - better error messages - Eric Auer - May 2003)
// (0.91b..g - Eric Auer 2003 / 0.91k ... Eric Auer 2004)
// (0.91t - message tuning - EA 2005)
// (0.91u - label charset check, Watcom / ... country support - EA 2005)
// (0.91v - year 2006, nicer Ask_User_To_Insert_Disk - EA 2006)
// Written By:  Brian E. Reifsnyder
// Copyright:  2002-2006 under the terms of the GNU GPL, Version 2
// Module Name:  userint.c
// Module Description:  User interfacing functions.
*/

/* #include <stdlib.h> */
/* no // comments for Turbo C! -ea */


#define USERINT

#include <dos.h>
#include <ctype.h>	/* toupper */
#include <string.h>	/* strlen  */
#include <io.h>		/* write (to stderr) */

#include "format.h"
#include "userint.h"


#define KEYGET_ECHO regs.h.ah = 0x07; /* Get keypress, upcase and echo */ \
  intdos(&regs, &regs); \
  if (!regs.h.al) { \
    regs.h.ah = 0x07; intdos(&regs, &regs); regs.h.al = 0x00; \
  } /* ignore extended keys */ \
  regs.h.al = toupper(regs.h.al); \
  if (regs.h.al>7) printf("%c",regs.h.al)
  /* 0.91v: suppress displaying of a few control chars - like 3, Ctrl C */



void ASCII_CD_Number(unsigned long number)
{
  int comma_counter;
  int index;
  int right_index;
  int shift_counter;
  int shift_register;
  int shift_temp_register;
  int start_shift;
  unsigned char dsep, ksep; /* decimal- and kilo-separator */

#if defined(__TURBOC__)
  struct country mycountry; /* NLS stuff added 0.91k */
  /* we assume that this is DOS 2.11+: offset 7 1000s-sep 9 decsep    */
  /* also in this buffer: date format, 12<->24 clock, currency stuff, */
  /* date sep (. or / or -), time sep (:) and lots of other things    */

  ksep = ','; /* or . */
  dsep = '.'; /* or , */
  if ( country(0 /* current */, &mycountry) )
    {
      ksep = mycountry.co_thsep[0];
      dsep = mycountry.co_desep[0];
    }
#else
  char mycountry[48];	/* probably 34 bytes are enough... */

  ksep = ','; /* or . */
  dsep = '.'; /* or , */
  regs.x.ax = 0x3800;
  sregs.ds = FP_SEG(&mycountry);
  regs.x.dx = FP_OFF(&mycountry);
  intdosx(&regs,&regs,&sregs);
  if (regs.x.cflag == 0) {
    ksep = mycountry[7];	/* 1000's separator */
    dsep = mycountry[9];	/* decimal separator */
  }
#endif

  memset((void *)&ascii_cd_number[0], 0, sizeof(ascii_cd_number));

  ultoa(number, ascii_cd_number, 10);

  /* Add Commas */
  index = 13;
  right_index = 13;
  start_shift = FALSE;

  do
    {
    if (ascii_cd_number[index]>0) start_shift = TRUE;

    if (start_shift==TRUE)
      {
      ascii_cd_number[right_index] = ascii_cd_number[index];
      ascii_cd_number[index] = 0;
      right_index--;
      }

    index--;
    } while (index>=0);

  comma_counter = 0;
  index = 13;
  do
    {
    comma_counter++;

    if (ascii_cd_number[index]==0)
      {
      comma_counter = 5;
      ascii_cd_number[index] = ' ';
      }

    if (comma_counter==4)
      {
      shift_counter = index-1;
      shift_register = ascii_cd_number[index];
      ascii_cd_number[index] =  ksep; /* 1000s separator */
      do
	{
	shift_temp_register = ascii_cd_number[shift_counter];
	ascii_cd_number[shift_counter] = shift_register;
	shift_register = shift_temp_register;

	shift_counter--;
	} while (shift_counter>=0);

      comma_counter = 0;
      }

    index--;
    } while (index>=0);

  ascii_cd_number[14] = 0;

  /* cheat: add decimal separator after end of ascii_cd_number string */
  ascii_cd_number[15] =  dsep;

} /* ASCII_CD_Number */


void Ask_User_To_Insert_Disk()
{
  printf(catgets(catalog, 9, 0, " Insert new diskette for drive %c:\n"), param.drive_letter[0]);
  if (!isatty(1)) 
    write(2, catgets(catalog, 9, 1, " Insert new disk please,\n"), strlen(catgets(catalog, 9, 1, " Insert new disk please,\n")));
  write(isatty(1) ? 1 : 2,
    catgets(catalog, 9, 2, " Press ENTER when the right disk is in drive..."), 
	strlen(catgets(catalog, 9, 2, " Press ENTER when the right disk is in drive...")));
  /* write to STDERR for the case of STDOUT being redirected */
  if (!isatty(1)) write(2, catgets(catalog, 1, 0, "\n"), 1);

  /* Wait for a key */

  regs.h.ah = 0x08;
  intdos(&regs, &regs);

  printf(catgets(catalog, 1, 0, "\n"));
} /* Ask_User_To_Insert_Disk */


void Confirm_Hard_Drive_Formatting(int format) /* 0 unformat, 1 format */
{
  char const *yes = catgets(catalog, 1, 3, "YES");
  char const *no = catgets(catalog, 1, 4, "NO");
  char const *yes_response;
  char yes_no_buffer[32];
  /* ensure message fits in buffer, if not make sure what we say matches what we check */
  if (strlen(yes)+strlen(no)+6 > sizeof(yes_no_buffer))
  {
    yes = "YES";
	no = "NO";
  }
  sprintf(yes_no_buffer, " (%s/%s)?", yes, no);

  if (!format)
    {
      printf(catgets(catalog, 10, 0, "UNFORMAT will revert your root directory and FAT to a\n"));
      printf(catgets(catalog, 10, 1, "previously recorded state. This can seriously mess up things!\n"));
    }
  printf(catgets(catalog, 1, 0, "\n"));
  printf(catgets(catalog, 10, 2, " WARNING: ALL DATA ON %s\n"),
    (param.drive_type==HARD) ? catgets(catalog, 10, 3, "NON-REMOVABLE DISK") : catgets(catalog, 10, 4, "FLOPPY DISK"));
  printf( (format) ? catgets(catalog, 10, 5, " DRIVE %c: WILL BE LOST!") : catgets(catalog, 10, 6, " DRIVE %c: MIGHT GET LOST!"),
    param.drive_letter[0]);
  printf(catgets(catalog, 10, 7, " PLEASE CONFIRM!\n"));

  /* printf(" Proceed with (Un)Format (YES/NO)? "); */
  /* write to STDERR for the case of STDOUT being redirected */
  if (format)
  {
    write(isatty(1) ? 1 : 2, catgets(catalog, 10, 8, "Proceed with Format"), 
      strlen(catgets(catalog, 10, 8, "Proceed with Format")));
  } else {
    write(isatty(1) ? 1 : 2, catgets(catalog, 10, 9, "Proceed with unFormat"),
      strlen(catgets(catalog, 10, 9, "Proceed with unFormat")));
  }
  write(isatty(1) ? 1 : 2, yes_no_buffer, strlen(yes_no_buffer));

  /* loop through and see if what user types matches */
  for(yes_response = yes; *yes_response; yes_response++)
  {
    KEYGET_ECHO;
	/* does character type match expected character? */
    if ( regs.h.al != *yes_response )
    {
      printf(catgets(catalog, 1, 0, "\n"));
      /* assume if 1st letter not a match then they chose No otherwise elaborate on what to type */
	  if ((yes_response - yes) != 0)
        printf(catgets(catalog, 10, 10, "You have to type the whole word %s to confirm."), yes);
      Exit(5,10);	/* extended error can be 10 through 12, 10 on Y, 11 on YE, 12 on YES mismatch */
    }
  }
  KEYGET_ECHO;  /* (usually the ENTER after YES - accept anything here) */
  printf(catgets(catalog, 1, 0, "\n")); /* enter is only \r, so we still need the \n ... */
} /* Confirm_Hard_Drive_Formatting */


int isLabelChar(int ch)		/* return non-0 if char is okay for label: 0.91u+ */
{				/* must check & 0x80 FIRST, to avoid SIGN hassles */
  if (ch & 0x80)    return 3;	/* for some reason, "ASCII" 128-255 ARE all nice  */
  /* label chars for MS FORMAT. Beware, they are all codepage / country specific! */
  if (ch < ' ')     return 0;	/* control chars are NOT okay */
  if (isalnum (ch)) return 1;	/* A-Z a-z 0-9 ARE all okay */
  /* Other rejected chars: " & ()*+, ./ :;<=>? [\]^ | and ASCII 127 */
  if (strchr ("!#$%'-@_{}~", ch))	/* ! #$% ' - @ _ { }~ */
                    return 2;	/* certain punctuation chars ARE okay */
                    return 0;	/* other rejected chars reach this return 0 line. */
} /* isLabelChar */


int Ask_Label(char * str)
{
  int label_len;

  write(isatty(1) ? 1 : 2,
    catgets(catalog, 11, 0, "Please enter volume label (max. 11 chars):"), 
	strlen(catgets(catalog, 11, 0, "Please enter volume label (max. 11 chars):")));
  write(isatty(1) ? 1 : 2, " ", 1);
  /* write to STDERR for the case of STDOUT being redirected */

  label_len = 0;
  str[0] = '\0';  /* Warning: str length is defined in format.h as char[12] */
  while (1==1) {
      KEYGET_ECHO;	/* (backspace is nondestructive in this _ECHO) */

      if (regs.h.al == 3)			/* ctrl-c */
          Exit(3,3);				/* aborted by user */
		  
      if (regs.h.al == 0)			/* extended key, eg up/down/left/right */
          continue;					/* ignore it */
	

#if 0 /* pre 0.91u version: A-Z 0-9 and ASCII 0x20-0x2f and 0x3a-0x40 */
!      if ((regs.h.al > 'Z') ||
!          ( (regs.h.al < ' ') && (regs.h.al != 8) && (regs.h.al != 13) ) ||
!          ( (label_len >= 11) && (regs.h.al != 13) && (regs.h.al != 8) ) )
#else /* new 0.91t version: A-Z 0-9, "ASCII" 0x80-0xff and some punctuation */
       if ( ( (!isLabelChar (regs.h.al)) || (label_len >= 11) ) &&
            (regs.h.al != 8) && (regs.h.al != 13) )
#endif
        {
          printf("\010 \010\007");		/* beep! (and wipe...) */
          continue;				/* skip if too long or */
        }					/* if invalid char...  */

      switch (regs.h.al)
        {
          case 8:  if (label_len) {
                  label_len--;			/* backspace */
                  printf(" \010");		/* really wipe */
              } else
                  printf(" ");			/* move the cursor back */
              break;
          case 13: printf("\n");		/* enter */
              if (label_len==0)
                  printf(catgets(catalog, 11, 1, "No label, disk will have no creation timestamp.\n"));
	      return label_len;			/* length of user input */          
              /* break; */
          default:
              str[label_len] = regs.h.al;	/* key */
              label_len++;
              str[label_len] = '\0';		/* terminating zero */
        } /* switch */
    };
} /* Ask_Label */


void Critical_Error_Handler(int source, unsigned int error_code)
{
  unsigned int error_code_high = (error_code & 0xff00) >> 8;
  unsigned int error_code_low  = error_code & 0x00ff;

  if (source==BIOS) {
    error_code_high = error_code_low;
    error_code_low  = 0x00;
  }
  printf(catgets(catalog, 12, 0, "\n Critical error during %s disk access\n"),
    (source==BIOS) ? "INT 13" : "DOS");

  /* Display Status Message. */

  if (source==BIOS) {
    printf(" INT 13 status (hex): %02x", error_code_high);

    printf("\n   Bits: ");
    /* changed to allow bit fields -ea */
    if (error_code_high == 0x00) printf("none ");
    if (error_code_high & 0x01) printf("bad command ");
    if (error_code_high & 0x02) printf("bad address mark ");
    if (error_code_high & 0x04) printf("sector not found ");
    if (error_code_high & 0x08) printf("DMA troubles ");
    if (error_code_high & 0x10) printf("data error (bad CRC) ");
    if (error_code_high & 0x20) printf("controller failed ");
    if (error_code_high & 0x40) printf("seek operation failed ");
    if (error_code_high & 0x80) printf("timeout / no response ");

    printf("\n   Description: "); /* *** marks popular errors */
    switch (error_code_high) {
      case 0x01: printf("invalid function or parameter"); break;
      case 0x02: printf("address mark not found"); break; /* *** */
      case 0x03: printf("DISK WRITE-PROTECTED"); break; /* *** */
      case 0x04: printf("read error / sector not found"); break; /* *** */
      case 0x05: printf("harddisk reset failed"); break;
      case 0x06: printf("FLOPPY CHANGED"); break; /* *** */
      case 0x07: printf("drive parameter error"); break;
      case 0x08: printf("DMA overrun"); break;
      case 0x09: printf("DMA crossed 64k boundary"); break; /* *** */
      case 0x0a: printf("bad sector on harddisk"); break;
      case 0x0b: printf("bad track on harddisk"); break;
      case 0x0c: printf("invalid media / track not found"); break; /* *** */
      case 0x0d: printf("illegal sectors / track (PS/2)"); break;
      case 0x0e: printf("harddisk address mark error"); break;
      case 0x0f: printf("harddisk DMA arbitration level overflow"); break;
      case 0x10: printf("READ ERROR (CRC/ECC wrong)"); break; /* *** */
      case 0x11: printf("read error corrected by CRC/ECC"); break;
      case 0x20: printf("controller failure"); break;
      case 0x31: printf("no media in drive"); break; /* (int 13 extensions) */
      case 0x32: printf("invalid CMOS drive type"); break; /* (Compaq) */
      case 0x40: printf("Seek failed"); break; /* *** */
      case 0x80: printf("timeout (drive not ready)"); break; /* *** */
      case 0x0aa: printf("timeout (harddisk not ready)"); break;
      case 0x0b0: printf("volume not locked in drive"); break; /* int 13 ext */
      case 0x0b1: printf("volume locked in drive"); break; /* int 13 ext */
      case 0x0b2: printf("volume not removable"); break; /* int 13 ext */
      case 0x0b3: printf("volume in use"); break; /* int 13 ext */
      case 0x0b4: printf("volume lock count overflow"); break; /* int 13 ext */
      case 0x0b5: printf("eject failed"); break; /* int 13 ext */
      case 0x0b6: printf("volume is read protected"); break; /* int 13 ext */
      case 0x0bb: printf("general harddisk error"); break;
      case 0x0cc: printf("harddisk WRITE ERROR"); break;
      case 0x0e0: printf("harddisk status reg error"); break;
      case 0x0ff: printf("harddisk sense failed"); break;
      default: printf("(unknown error code)"); break;
    } /* switch */
  } /* BIOS */
  else
  { /* DOS */

    printf(" DOS driver error (hex): %02x", error_code_low);

    printf("\n   Description: ");
    switch (error_code_low) {
      case 0x00: printf("write-protect error / none"); break;
      case 0x01: printf("unknown unit for driver"); break;
      case 0x02: printf("drive not ready"); break;
      case 0x03: printf("unknown command"); break;
      case 0x04: printf("data error (bad CRC)"); break;
      case 0x05: printf("bad request structure length"); break;
      case 0x06: printf("seek error"); break;
      case 0x07: printf("unknown media type"); break;
      case 0x08: printf("sector not found"); break;
      case 0x09: printf("printer out of paper"); break;
      case 0x0a: printf("write fault"); break;
      case 0x0b: printf("read fault"); break;
      case 0x0c: printf("general failure"); break;
      case 0x0d: printf("sharing violation"); break;
      case 0x0e: printf("lock violation"); break;
      case 0x0f: printf("invalid disk change"); break;
      case 0x10: printf("FCB unavailable"); break;
      case 0x11: printf("sharing buffer overflow"); break;
      case 0x12: printf("code page mismatch"); break;
      case 0x13: printf("out of input"); break;
      case 0x14: printf("disk full"); break;
      default: printf("(unknown error code)"); break;
    } /* switch */
    error_code_high = error_code_low;	/* for return value */
  } /* DOS */

  /* *** We should probably allow an IGNORE or RETRY in *some* cases! *** */

  printf(catgets(catalog, 12, 1, "\n Program terminated.\n"));

  Exit(4, (128 | error_code_high) );	/* critical error handler */
} /* Critical_Error_Handler */


/* 0.91k - avoid overflow if FAT32 */
void Display_Drive_Statistics()
{

#define DDS_unitstring ((param.fat_type == FAT32) ? "kbytes" : "bytes")
#define DDS_factor(x) ((param.fat_type == FAT32) ? ((x)>>1) : ((x) * \
  drive_statistics.bytes_per_sector))
/* a cheat: ASCII_CD_Number puts the decimal point at index 15... */
#define DDS_dp ((param.fat_type == FAT32) ? ascii_cd_number[15] : ' ')
#define DDS_half(x) ((param.fat_type == FAT32) ? (((x) & 1) ? "5" : "0") : "")

  if ((drive_statistics.bytes_per_sector != 512) && (param.fat_type == FAT32))
    printf("Not 512 bytes/sector - stats will be wrong.\n");

  /* changed in 0.91h */
  /* avail = data area size (w/o root dir), total = "diskimage" size */
  drive_statistics.allocation_units_available_on_disk
    = drive_statistics.sect_available_on_disk
    / drive_statistics.sect_in_each_allocation_unit;

  drive_statistics.sect_available_on_disk -=
    ( drive_statistics.allocation_units_with_bad_sectors
      * drive_statistics.sect_in_each_allocation_unit );

  ASCII_CD_Number(DDS_factor(drive_statistics.sect_total_disk_space));
  printf("\n%13s%c%s %s total disk space (disk size)\n",
    ascii_cd_number, DDS_dp, DDS_half(drive_statistics.sect_total_disk_space),
    DDS_unitstring);

  if (drive_statistics.bad_sectors > 0) /* changed 0.91c */
    {
    ASCII_CD_Number(DDS_factor(drive_statistics.bad_sectors));
    printf("%13s%c%s %s in bad sectors\n",
      ascii_cd_number, DDS_dp, DDS_half(drive_statistics.bad_sectors),
      DDS_unitstring);
    ASCII_CD_Number(DDS_factor(drive_statistics.sect_total_disk_space -
      drive_statistics.sect_available_on_disk));
    printf("%13s%c%s %s in clusters with bad sectors\n",
      ascii_cd_number, DDS_dp, DDS_half(drive_statistics.sect_total_disk_space -
        drive_statistics.sect_available_on_disk), DDS_unitstring);
    }

  ASCII_CD_Number(DDS_factor(drive_statistics.sect_available_on_disk));
  printf("%13s%c%s %s available on disk (free clusters)\n",
    ascii_cd_number, DDS_dp, DDS_half(drive_statistics.sect_available_on_disk),
    DDS_unitstring);

  printf("\n");

  ASCII_CD_Number(DDS_factor(drive_statistics.sect_in_each_allocation_unit));
  printf("%13s%c%s %s in each allocation unit.\n",
    ascii_cd_number, DDS_dp, DDS_half(drive_statistics.sect_in_each_allocation_unit),
    DDS_unitstring);

  ASCII_CD_Number(drive_statistics.allocation_units_available_on_disk);
  printf("%13s%s allocation units on disk.\n", ascii_cd_number,
    (param.fat_type == FAT32) ? "  " : "");

  if (drive_statistics.allocation_units_with_bad_sectors > 0) /* 0.91c */
    {
    ASCII_CD_Number(drive_statistics.allocation_units_with_bad_sectors);
    printf("%13s%s of the allocation units marked as bad\n", ascii_cd_number,
      (param.fat_type == FAT32) ? "  " : "");
    }

  printf(catgets(catalog, 13, 0, "\n Volume Serial Number is %04X-%04X\n"),
   drive_statistics.serial_number_high, drive_statistics.serial_number_low);
} /* Display_Drive_Statistics */


void Display_Invalid_Combination()
{
  printf(catgets(catalog, 14, 0, "\n Invalid combination of options... please read help. Aborting.\n"));
  Exit(4,2);
} /* Display_Invalid_Combination */


void Key_For_Next_Page(void);

void Key_For_Next_Page()
{
  if (!isatty(1))
    return; /* redirected? then do not wait. */
    /* interesting: redirection to MORESYS (>MORE$) still is a TTY   */
    /* redirection to a file is not a TTY, so waiting is avoided :-) */

  printf(catgets(catalog, 14, 1, "-- press enter to see the next page or ESC to abort  --"));

  /* Get keypress */
  regs.h.ah = 0x07;
  intdos(&regs, &regs);
  if (regs.h.al == 27)
    {
      printf(catgets(catalog, 14, 2, "\nAborted at user request.\n"));
      Exit(3,13);
    }

  printf("\n\n");

} /* Key_For_Next_Page */


void Print_Messages_With_Pauses(nl_catd catalog, int setnum, char const *messages[])
{
  int msgnum = 0;
  int use_cats = (catgets(catalog, setnum, 0, NULL) != NULL);
  char const * msg;
  
  for (msg=(use_cats)?catgets(catalog, setnum, msgnum, NULL):messages[msgnum]; 
       msg != NULL; 
       msgnum++, msg=(use_cats)?catgets(catalog, setnum, msgnum, NULL):messages[msgnum])
  {
    printf("%s", msg);
	if (msgnum && !(msgnum % 22)) /* every 23rd line after initial line */
    {
      if (memcmp(msg, "\n", 2) != 0) printf("\n");  /* add blank unless just printed a blank line */
      Key_For_Next_Page();
    }
  }
} /* Print_Messages_With_Pauses */


/* detailed help screen messages */
const char const * detailed_help[] = {
  "This FORMAT is made for the http://www.freedos.org/ project.\n",
  "  See http://www.gnu.org/ for information about GNU GPL license.\n",
  "Made in 1999-2003 by Brian E. Reifsnyder <reifsnyderb@mindspring.com>\n",
  "  Maintainer for 0.90 / 0.91 2003-2006: Eric Auer <eric@coli.uni-sb.de>\n",
  "Contributors: Jan Verhoeven, John Price, James Clark, Tom Ehlert,\n",
  "  Bart Oldeman, Jim Hall and others. Not to forget all the testers!\n",
  "\n",
  "Switches and additional features explained:\n",
  "/D (debug) and /Y (skip confirmation request) are always allowed.\n",
  "/B (reserve space for sys) is dummy and cannot be combined with /S (sys)\n",
  "/V:label is not for 160k/320k disks. The label stores the format date/time.\n",
  "\n",
  "Size specifications only work for floppy disks. You can use\n",
  "either /F:size (in kilobytes, size 0 displays a list of allowed sizes)\n",
  "or     /T:tracks /N:sectors_per_track\n",
  "or any combination of /1 (one-sided, 160k/180k),\n",
  "                      /8 (8 sectors per track, 160k/320k, DOS 1.x)\n",
  "                  and /4 (format 160-360k disk in 1200k drive)\n",
  "\n",
  "To suppress the harddisk format confirmation prompt, use    /Z:seriously\n",
  "To save only unformat (mirror) data without formatting, use /Z:mirror\n",
  "To UNFORMAT a disk for which fresh mirror data exists, use  /Z:unformat\n",
  "\n",
  "Modes for FLOPPY are: Tries to use quick safe format. Use lowlevel format\n",
  "  only when needed. Quick safe format saves mirror data for unformat.\n",
  "Modes for HARDDISK are: Tries to use quick safe format. Use quick full\n",
  "  format only when needed. Quick full format only resets the filesystem\n",
  "If you want to force lowlevel format (floppy) or want to have the whole\n",
  "  disk surface scanned and all contents wiped (harddisk), use /U.\n",
  "  FORMAT /Q /U is quick full format (no lowlevel format / scan / wipe!)\n",
  "  FORMAT /Q is quick save format (save mirror data if possible)\n",
  "    the mirror data will always overwrite the end of the data area!\n",
  "  FORMAT autoselects a mode (see above) if you select neither /Q nor /U\n",
  "\n",
  "Supported FAT types are: FAT12, FAT16, FAT32, all with mirror / unformat.\n",
  "Supported floppy sizes are: 160k 180k 320k 360k and 1200k for 5.25inch\n",
  "  and 720k and 1440k (2880k never tested so far) for 3.5inch drives.\n",
#if 0 /* should be obvious */
  "DD drives are limited to 360k/720k respectively. 2880k is ED drives only.\n",
#endif
  "Supported overformats are: 400k 800k 1680k (and 3660k) with more sectors\n",
  "  and 1494k (instead of 1200k) and 1743k (and 3486k) with more tracks, too.\n",
  "  More tracks will not work on all drives, use at your own risk.\n",
  "  Warning: older DOS versions can only use overformats with a driver.\n",
  "  720k in 1440k needs 720k media. Use 360k drive to format 360k.\n",
  "\n",
/* formatting to 360k in 1200k drive gives weak format: 360k drive has broad tracks */
/* and 1200k drive has narrow read / write heads. 1200k drive can read 360k disks   */
/* formatted with 1200k drive, though, but you'll prefer to format to 1200k there.  */
  "For FAT32 formatting, you can use the /A switch to force 4k alignment.\n",
#if 0
// printf("WARNING: Running FORMAT for FAT32 under Win98 seems to create broken format!\n");
#endif
  NULL
};

/* Help Routine (removed legacy stuff in 0.91c, but it is still parsed) */
/* There should be an help file which explains everything in detail,    */
/* including the (hardly ever used but still supported) legacy options! */
/* 0.91n - if detailed is non-zero, display multi-page help screen. */
void Display_Help_Screen(int detailed)
{
  printf(catgets(catalog, 0, 0, "FreeDOS %6s Version %s\n"),NAME,VERSION);
  printf(catgets(catalog, 0, 1, "Written by Brian E. Reifsnyder, Eric Auer and others.\n"));
  printf(catgets(catalog, 0, 2, "Copyright 1999 - 2024 under the terms of the GNU GPL, Version 2+.\n\n"));

  if (detailed)
    printf(catgets(catalog, 2, 0, "Syntax (see documentation for more details background information):\n\n"));
  else
    printf(catgets(catalog, 2, 1, "Syntax (see documentation or use /Z:longhelp for more options):\n\n"));

#if LEGACY_HELP /* with legacy stuff */
  printf(catgets(catalog, 2, 3, "FORMAT drive: [/V[:label]] [/Q] [/U] [/F:size] [/B | /S] [/D]\n"));
  printf(catgets(catalog, 2, 4, "FORMAT drive: [/V[:label]] [/Q] [/U] [/T:tracks /N:sectors] [/B | /S] [/D]\n"));
  printf(catgets(catalog, 2, 5, "FORMAT drive: [/V[:label]] [/Q] [/U] [/4] [/B | /S] [/D]\n"));
  printf(catgets(catalog, 2, 6, "FORMAT drive: [/Q] [/U] [/1] [/4] [/8] [/B | /S] [/D]\n\n"));
#else /* new - without legacy stuff */
  printf(catgets(catalog, 2, 7, "FORMAT drive: [/V[:label]] [/Q] [/U] [/F:size] [/S] [/D]\n"));
  printf(catgets(catalog, 2, 8, "FORMAT drive: [/V[:label]] [/Q] [/U] [/T:tracks /N:sectors] [/S] [/D]\n"));
  /* the /4 option is a legacy shorthand for size selection: 360k in 1.2M drive */
  /* (drive type detection and "double stepping" setting are automatic on ATs.) */
  printf(catgets(catalog, 2, 9, "FORMAT drive: [/V[:label]] [/Q] [/U] [/4] [/S] [/D]\n\n"));
#endif

  printf(catgets(catalog, 2, 10, " /V:label   Specifies a volume label for the disk, stores date and time of it.\n"));
  printf(catgets(catalog, 2, 11, " /S         Calls SYS to make the disk bootable and to add system files.\n"));

#if LEGACY_HELP /* legacy, DOS 1.x (/B cannot be combined with /S) */
  printf(catgets(catalog, 2, 12, " /B         Kept for compatibility, formerly reserved space for the boot files.\n"));
#endif  
  printf(catgets(catalog, 2, 13, " /D         Be very verbose and show debugging output. For bug reports.\n"));

  printf(catgets(catalog, 2, 14, " /Q         Quick formats the disk. If not combined with /U, can be UNFORMATed\n"));
  printf(catgets(catalog, 2, 15, "            and preserves bad cluster marks (/Q /U does not).\n"));
  /* preserving the bad cluster list is new in 0.91k */
  printf(catgets(catalog, 2, 16, " /U         Unconditionally formats the disk. Lowlevel format if floppy disk.\n"));

  printf(catgets(catalog, 2, 17, " /F:size    Specifies the size of the floppy disk to format. Normal sizes are:\n"));
  printf(catgets(catalog, 2, 18, "            360, 720, 1200, 1440, or 2880 (unit: kiloBytes). /F:0 shows a list.\n"));
  printf(catgets(catalog, 2, 19, " /4         Formats a 360k floppy disk in a 1.2 MB floppy drive.\n"));
  printf(catgets(catalog, 2, 20, " /T:tracks  Specifies the number of tracks on a floppy disk.\n"));
  printf(catgets(catalog, 2, 21, " /N:sectors Specifies the number of sectors on a floppy disk.\n"));

#if LEGACY_HELP /* legacy, DOS 1.x */
  printf(catgets(catalog, 2, 22, " /1         Formats a single side of a floppy disk (160k / 180k).\n"));
  printf(catgets(catalog, 2, 23, " /8         Formats a 5.25\" disk with 8 sectors per track (160k / 320k).\n"));
#endif  

  if (!detailed) return; /* stop here for normal, short, help screen */

  printf(catgets(catalog, 1, 0, "\n"));
  Key_For_Next_Page();  
  Print_Messages_With_Pauses(catalog, 3, detailed_help);
} /* Display_Help_Screen */


void Display_Percentage_Formatted(unsigned long percentage)
{
#if I_AM_ALAIN
  /* support abort with Ctrl-C (can spoil ESC handling) */
  /* note that you can always abort with Ctrl-Break...  */
  while (my_kbhit()) { if (my_getch() == 3) Exit(3,3); }
#endif

  if (debug_prog==TRUE)
    {
    printf("%3d%% ", percentage);
    }
  else
    {
    if (isatty(1 /* stdout */))
      printf("%3d percent completed.\r", percentage); /* on screen */
    else
      printf("%3d percent completed.\n", percentage); /* for redirect */
    /* \r re-positions cursor back to the beginning of the line */
    }
} /* Display_Percentage_Formatted */


void IllegalArg(char *option, char *argptr)
{
    printf("Parameter value not allowed - %s%s\n", option, argptr);
    Exit(4,14);
} /* IllegalArg */

