/*
// Program:  Format
// Version:  0.91v
// (0.90b/c/d/e/f - mixed improvements - Eric Auer 2003)
// (0.91b..i - Eric Auer 2003 / 0.91k ... Eric Auer 2004)
// (0.91t - improved SYS call, subst/net/... detection - Eric Auer 2005)
// (0.91u - label char checking - Eric Auer 2005)
// (0.91v - no truename on a:/b:, exit 1/2 style, boot plausibility - EA 2006)
// Written By:  Brian E. Reifsnyder
// Copyright:  2002-2006 under the terms of the GNU GPL, Version 2
// Module Name:  main.c
// Module Description:  Main Module
*/

#define MAIN


#if defined(__TURBOC__)
#include <dir.h>
#endif

#include <stdio.h>
#include <ctype.h>			/* (jh) for isalpha, ... */
#include <string.h>			/* strncpy, strlen */
#include <dos.h>
#include <io.h>				/* write */
#include <process.h>			/* P_WAIT for spawnl */

#include "format.h"
#include "hdisk.h"

#include "init.h"
#include "userint.h"
#include "createfs.h"
#include "uformat.h"
#include "btstrct.h"
#include "savefs.h"

#include "driveio.h"			/* for drive lock / unlock */
#include "floppy.h"

#include "getopt.h"			/* (jh) support for getopt */

void Check_Remote_Subst(void);
char Check_For_Format(void);
void Write_System_Files(void);


void Initialization(void);
void Unconditional_Format(void);
void Record_Bad_Clusters(void);
void Set_Floppy_Media_Type(void);
void Enable_Disk_Access(void);



void Check_Remote_Subst(void)
{
  char prename[4] = { 'X', ':', '\\', 0 };
  char postname[128];

  regs.x.ax = 0x4409;			/* "check if block device remote" */
  regs.h.bl = param.drive_number + 1;	/* 0 is default, 1 is A: ... */
  intdos(&regs, &regs);
  if ((regs.x.dx & 0x9000) != 0)	/* 8000: subst, 1000: remote */
  {
    printf("Cannot format remote or SUBSTed drives (code %04x). Aborting.\n",
      regs.x.dx);
    Exit(4,60);
  }

  if (param.drive_number < 2) {	/* no truename for a: or b: because we ... */
    if (debug_prog==TRUE) printf("[DEBUG]  Skipped ASSIGN check for diskette drive.\n");
    return;			/* ... would get critical error on unformatted disks */
  }

  regs.x.ax = 0x6000;	/* truename */
  prename[0] = param.drive_letter[0];
  regs.x.si = FP_OFF(prename);
  sregs.ds = FP_SEG(prename);
  regs.x.di = FP_OFF(postname);
  sregs.es = FP_SEG(postname);
  intdosx(&regs, &regs, &sregs);
  if (regs.x.cflag != 0)
  {
    printf("Invalid Drive! Aborting.\n");
    Exit(4,61);
  }
  if (stricmp(prename, postname))
  {
    printf("Cannot format ASSIGNed, JOINed or SUBSTed drives. Aborting.\n");
    if (debug_prog==TRUE) printf("[DEBUG]  Truename: %s\n", postname);
    Exit(4,62);
  }
} /* Check_Remote_Subst */


/* Check to see if the media is formatted.  */
/* Returns TRUE if formatted and FALSE if it is not formatted. */
/* Non-DOS formats count as formats as well! (Eric) */
char Check_For_Format(void)
{
  union REGS regs;

  regs.h.ah = 0x0d;	/* this is in Check_For_Format */
  intdos(&regs,&regs); /* flush buffers, reset disk system */
  if (debug_prog==TRUE) printf("[DEBUG]  Test-reading boot sector.\n");

  /* as int 21.32 does not work for FAT32, we ALWAYS read the boot sector */
  param.fat_type = FAT16; /* Check_For_Format: guess "not FAT32" first */
  regs.x.ax = Drive_IO(READ, 0, -1);

  if ((regs.x.ax != 0) && (param.drive_type != FLOPPY))
    {
/*
 *    if (debug_prog==TRUE) printf("[DEBUG]  Trying again in FAT32 mode.\n");
 */
      if (debug_prog==TRUE) printf(" maybe FAT32?\n");
      param.fat_type = FAT32; /* Check_ForFormat: try FAT32 Drive_IO next */
      regs.x.ax = Drive_IO(READ, 0, -1);
      if (regs.x.ax != 0)
        param.fat_type = FAT16; /* Check_For_Format: revert to FAT1x Drive_IO */
    }
      
  if (regs.x.ax != 0) /* could not even read boot sector */
    {
      if (debug_prog==TRUE) printf("[DEBUG]  Error code: 0x%x\n", regs.x.ax);
      /* bits: 80 timeout 40 seek 20 controller fail 10 CRC error        */
      /* 8 DMA error 4 sector not found 2 bad address mark 1 bad command */
      /*  (int 26 would have special extra code 3, write protect error)  */
      printf("  Boot sector unreadable, disk not yet formatted\n");
      return FALSE;
    }

    /* test if sectors per track are 1..63, if the 55 aa magic is, */
    /* present and if the bytes per sector word has the value 512  */
    /* added 0.91v as MS DOS criterrs on int 21.32 if no FAT file- */
    /* system is present on disk. problem found by Alain Mouette.  */
  if ((sector_buffer[0x18]>63) || (sector_buffer[0x18]==0) ||
    (sector_buffer[0x1fe]!=0x55) || (sector_buffer[0x1ff]!=0xaa) ||
    (sector_buffer[0x0b]!=0) || (sector_buffer[0x0c]!=2))
    {
      printf(" Boot sector contents implausible, disk not yet FAT formatted\n");
      return FALSE;
    }

  if (param.fat_type == FAT32)
    {
      /* if (debug_prog==TRUE) printf("[DEBUG]  Checking xBPB (FAT32)\n"); */
      regs.x.ax = 0x7302; /* this is in Check_For_Format */ /* read xBPB */
      regs.h.dl = param.drive_number + 1; /* 0 default 1 A: 2 B: ... */
      regs.x.di = FP_OFF(&sector_buffer[0]);
      sregs.es  = FP_SEG(&sector_buffer[0]);
      regs.x.cx = 0x3f; /* amount of to-be-used buffer size */
      regs.x.si = 0; /* (can be a magic value to get more data) */
      regs.x.cflag = 0;
      intdosx(&regs,&regs,&sregs); /* read xBPB for that drive */
      if (regs.x.cflag == 0)
        regs.x.ax = 0; /* call worked */
    }
  else
    {
      /* if (debug_prog==TRUE) printf("[DEBUG]  Checking DPB/BPB (FAT1x)\n"); */
      regs.h.ah = 0x32; /* this is in Check_For_Format */ /* read BPB/DPB */
      regs.h.dl = param.drive_number + 1; /* 0 default 1 A: 2 B: ... */
      intdosx(&regs,&regs,&sregs); /* re-read info for that drive */
      /* this can cause a critical error if boot sector not readable! */
      regs.h.ah = 0;
    }

  if (regs.x.ax == 0)
    {
    /* could analyze returned DPB  - pointer is DS:BX if FAT16/FAT12 */
    /* could analyze returned xBPB - it is in sector_buffer if FAT32 */
    if (debug_prog==TRUE) printf("[DEBUG]  Existing format detected.\n");
    return TRUE;
    }
  else
    {
    printf("Invalid %sBPB (code 0x%x). NOT yet formatted.\n",
      ((param.fat_type == FAT32) ? "x" : ""), regs.x.ax);
    return FALSE; /* is this is a network drive or something? */
    }
} /* Check_For_Format */


/* Write System Files */
void Write_System_Files(void)
{
#ifdef __TURBOC__
  char * syspath = NULL;
  char sysarg0[3] = { 'x', ':', 0 };
#endif
  int retval;

#if defined(__TURBOC__)
  /* Check for the sys command. */
  syspath = searchpath("sys.com");
  if (syspath == NULL)
    syspath = searchpath("sys.exe");
  if (syspath == NULL)
  {
    printf("\nWARNING: No SYS in PATH - could not install system files!\n");
    return;
  }

  sysarg0[0] = param.drive_letter[0];
  printf("\nRunning SYS: %s %s\n", syspath, sysarg0);
  retval = spawnl(P_WAIT, syspath, syspath, sysarg0, NULL);
#else
  /* use less efficient / less safe style with a new shell: */
  /* Issue the command to write system files. */
  char sys[7] = {'s','y','s',' ','x',':',0};
  sys[4] = param.drive_letter[0];
  printf("\nRunning SYS in a shell: %s\n", sys);
  retval = system(sys);
#endif

  if (retval > 0) printf("\nSYS returned errorlevel %d.\n", retval);
  if (retval < 0) printf("\nWARNING: Running SYS failed.\n");
} /* Write_System_Files */


/*
/////////////////////////////////////////////////////////////////////////////
//  MAIN ROUTINE
/////////////////////////////////////////////////////////////////////////////
*/
void main(int argc, char *argv[])
{
  int ch;
  int index;
  int n;
  int found_format_sectors_per_track = 0;
  int found_format_heads = 0;

#define MIRROR 1
#define UNFORMAT 2

#define WHAT_FORMAT 1	/* new 0.91p, for custom "... completed" messages */
#define WHAT_QUICK 2
#define WHAT_SAFE 3
#define WHAT_UN 4
#define WHAT_MIRROR 5
  int what_completed = 0; /* new 0.91p */

  int special = 0;	/* mirror / unformat mode indicator */
  int align = 1;	/* new 0.91m */

  int drive_letter_found = FALSE;

  Initialization();


  param.n = FALSE;
  param.t = FALSE;
  param.v = FALSE;
  param.q = FALSE;
  param.u = FALSE;
  param.b = FALSE;
  param.s = FALSE;
  param.f = FALSE;
  param.one = FALSE;
  param.four = FALSE;
  param.eight = FALSE;
  debug_prog = FALSE;


  /* if FORMAT is typed without any options */

  if (argc == 1)
    {
    Display_Help_Screen(0); /* SHORT version */
    Exit(2,2); /* Exit(0,1); pre 0.91v */
    }

  /* (jh) check command line */

  while (  (index = getopt (argc, argv, "V:v:Z:z:QqUuBbSsYyAa148F:f:T:t:N:n:/")) != EOF)
    {
      switch(index)
	{
	case 'V':
	case 'v':
	  param.v = TRUE;
	  /* avoid overflow of param.volume_label (12 chars) */
	  /* need to skip over first character (':') */

	  strncpy (param.volume_label, optarg+1, 11);
	  param.volume_label[11] = '\0';

	  for (n = 0; param.volume_label[n] != '\0'; n++)
	    {
	      ch = param.volume_label[n];
	      if (!isLabelChar (ch))	/* new valid-character check 0.91u */
	        {
	        printf ("Illegal character in volume label: %c\n", ch);
	        printf ("Allowed chars are 0-9, A-Z, space, special / country-specific chars 128-255\n"
	                "and all of");
	        for (ch = ' '+1; ch < 128; ch++)
	          {
	            if ( (!isalnum (ch)) && (isLabelChar (ch)) )
	              printf (" %c", ch);
	          } /* enumerate legal chars */
	        printf ("\nbut no control chars and none of");
	        for (ch = ' '+1; ch < 128; ch++)
	          {
	            if (!isLabelChar (ch))
	              printf (" %c", ch);
	          } /* enumerate illegal chars */
	        printf ("\n");
	        Exit(0,2);
	        } /* illegal char in label */
	      param.volume_label[n] = toupper (ch);
	    } /* check label */
	  break;

        case 'Z': /* our getopt has no "long" support, so we use /Z:keyword */
	case 'z': /* 0.91l - extra (long) options */
	  if (!stricmp(optarg+1,"mirror")) /* +1 to skip initial ":" */
	    {
	    special = MIRROR;      /* take a new mirror data snapshot */
	    break;
	    }
	  if (!stricmp(optarg+1,"unformat"))
	    {
	    special = UNFORMAT;	   /* revert to mirrored state, dangerous! */
	    break;
	    }
	  if (!stricmp(optarg+1,"seriously"))
	    {
	    param.force_yes = TRUE + TRUE; /* User MEANS to format harddisk */
	    break;
	    }
	  if (!stricmp(optarg+1,"longhelp"))
	    {
            Display_Help_Screen(1); /* LONG version */
            Exit(2,2);	/* Exit(0,1); pre 0.91v */
	    }
	  printf("Invalid /Z:mode - valid: MIRROR, UNFORMAT, SERIOUSLY\n");
          Exit(2,2);	/* Exit(0,2); pre 0.91v */
          break;

	case 'Q': /* quick - flush metadata only */
	case 'q':
	  param.q = TRUE;
	  break;
	case 'U': /* unconditional - full format */
	case 'u':
	  param.u = TRUE;
	  break;
	case 'B': /* reserve space for system, deprecated */
	case 'b':
	  param.b = TRUE;
	  break;
	case 'S': /* run SYS command */
	case 's':
	  param.s = TRUE;
	  break;
	case 'Y': /* assume yes for confirmations - "undocumented" */
	case 'y':
	  if (param.force_yes == FALSE) param.force_yes = TRUE;
	  break;
	case 'D': /* debug mode, more verbose output */
	case 'd':
	  debug_prog = TRUE;
	  break;
	case 'A': /* 0.91m - align mode, force FAT size to 2k*n and */
	case 'a': /* so on to force data clusters to start at 4k*n  */
	  align = 8;
	  break;
	case '1': /* one side only */
	  param.one = TRUE;
	  break;
	case '4': /* 360k disk in 1.2M drive */
	  param.four = TRUE;
	  break;
	case '8': /* 8 sectors per track */
	  param.eight = TRUE;
	  break;

	case 'F':           /* /F:size */
	case 'f':           /* /F:size */
	  param.f = TRUE;
	  n = atoi (optarg + 1);

	  if ((n == 160) || (n == 180) || (n == 320) || (n == 360) ||
	      (n == 720) || (n == 1200) || (n == 1440) || (n == 2880) ||
              (n == 400) || (n == 800) || (n == 1680) || (n == 3360) ||
              (n == 1494) || (n == 1743) || (n == 3486))
	    {
	    param.size = n;
	    }
	  else
	    {
            printf("Standard: 160, 180, 320, 360, 720, 1200, 1440, 2880.\n");
            printf("Special:  400, 800, 1680, 3360,    1494, 1743, 3486.\n");
	    IllegalArg("/F",optarg);
	    }
	  break;

	case 'T': /* tracks (cylinders) */
	case 't': /* tracks (cylinders) */
	  param.t = TRUE;
	  n = atoi (optarg + 1);

	  if ((n == 40) || (n == 80) || (n == 83))
	    {
	    param.cylinders = n;
	    }
	  else
	    {
            printf("Ok: 40, 80. ???: 83.\n");
	    IllegalArg("/T",optarg);
	    }
	  break;

	case 'N': /* sectors per track */
	case 'n': /* sectors per track */
	  param.n = TRUE;
	  n = atoi (optarg + 1);

	  if ((n == 8) || (n == 9) || (n == 15) || (n == 18) || (n == 36) ||
              (n == 10) || (n == 21) || (n == 42))
	    {
	    param.sectors = n;
	    }
	  else
	    {
            printf("Standard: 8, 9, 15, 18, 36.\n");
            printf("Special:  10, 21, 42.\n");
	    IllegalArg("/N",optarg);
	    }
	  break;

	case '~':
	  param.drive_letter[0] = toupper (optarg[0]);
	  param.drive_number = param.drive_letter[0] - 'A';
	  param.drive_letter[1] = ':';
	  drive_letter_found = TRUE;
	  break;

        case '?':
          Display_Help_Screen(0); /* SHORT version */
          Exit(2,2);	/* Exit(0,1); pre 0.91v */

	case '/':			/* Ignore '/' in middle of option */
	  break;

	default:
	  printf("Unrecognized option: /%c\n", index);
	  Display_Help_Screen(0); /* SHORT version */
          Exit(2,2);	/* Exit(4,2); pre 0.91v */

	}       /* switch (index) */
    }         /* for all args (getopt) */


  if ( (argc > optind) && (isalpha (argv[optind][0])) &&
       (argv[optind][1] == ':') && (argv[optind][2] == '\0') &&
       (drive_letter_found == FALSE) )
    {
    param.drive_letter[0] = toupper (argv[optind][0]);
    param.drive_number = param.drive_letter[0] - 'A';
    param.drive_letter[1] = ':';
    }
  else
    if (drive_letter_found == FALSE)
      {
      printf("Must specify drive letter.\n");
      Exit(0,2);
      }

  /* (jh) done with parsing command line */



  /* if FORMAT is typed with a drive letter */

  if (debug_prog==TRUE) {
    printf("\n[DEBUG]  FORMAT " VERSION ", selected drive %c:\n",
      param.drive_letter[0]);
    printf("[DEBUG]  Sector buffer at %04x:%04x, track buffer at %04x:%04x\n",
      FP_SEG(sector_buffer), FP_OFF(sector_buffer),
      FP_SEG(huge_sector_buffer), FP_OFF(huge_sector_buffer) );
      /* moved this message to this place in 0.91s */
  }

  /* 0.91t - abort if subst, join, assign or remote drives */
  Check_Remote_Subst();		/* only has to be invoked once */

  /* Set the type of disk */
  if (param.drive_number>1)
    param.drive_type=HARD;
  else
    param.drive_type=FLOPPY;


  /* *** TODO: complain about ANY size determination if type HARD *** */


  /* Ensure that valid switch combinations were entered */
  if ( (param.b==TRUE) && (param.s==TRUE) )
    Display_Invalid_Combination();
    /* cannot reserve space for SYS and actually SYS at the same time */
  if ( (param.v==TRUE) && (param.eight==TRUE) )
    Display_Invalid_Combination();
    /* no label allowed if 160k / 320k */

  if ( ( (param.one==TRUE) || (param.four==TRUE) /* 360k in 1.2M drive */ ) &&
       ( (param.f==TRUE) || (param.t==TRUE) || (param.n==TRUE) ) )
    Display_Invalid_Combination();
    /* cannot combine size/track/sector override with 1-sided / 360k */

  if ( ( (param.t==TRUE) && (param.n!=TRUE) ) || 
       ( (param.n==TRUE) && (param.t!=TRUE) ) ) 
    {
    printf("You cannot combine /T and /N.\n");
    Display_Invalid_Combination();
    /* you must give BOTH track and sector arguments if giving either */
    }

  if ( (param.f==TRUE) && ( (param.t==TRUE) || (param.n==TRUE) ) )
    Display_Invalid_Combination();
    /* you can only give EITHER size OR track/sector arguments */
  
#if 0
  if ( ( (param.one==TRUE) || (param.four==TRUE) ) && (param.eight==TRUE) )
    Display_Invalid_Combination();
    /* 360k / one-sided are both not 8 sectors per track (360k is 40x2x9) */
#endif

  /* we do allow /8 to reach 160k (with /1) and 320k (with /4) */
  /* it is more the other way round: we do not want 8 sector/track > 320k! */


/* ... */


  if (param.one==TRUE) /* one-sided: 160k and 180k only */
    {
    param.sides = 1;
    param.cylinders = 40;
    /* *** this is actually handled in Set_Floppy_Media_Type mostly *** */
    }

  if (param.four==TRUE) /* 360k in 1200k drive */
    { 
    param.cylinders = 40;
    param.sectors = 9;
    }

  if (param.eight==TRUE) /* DOS 1.0 formats: 160k and 320k only */
    {
    param.sectors = 8;
    }


  Lock_Unlock_Drive(1); /* lock drive (needed in Win9x / DOS 7.x) */
  			/* FIRST lock, file system still enabled. */



next_disk:

  /* User interaction. */
  if (param.drive_type==FLOPPY && param.force_yes==FALSE)
    Ask_User_To_Insert_Disk();

  if (!special)
    {
      if (param.drive_type==HARD && (param.force_yes!=(TRUE+TRUE)) )
        Confirm_Hard_Drive_Formatting(1); /* 1 means "format" */
   /* disabled harddisk "/Y" forced confirmation in 0.91b unless ZAPME -ea */
   /* replaced by "/Z:seriously" in 0.91l -ea */
    }
  else
    {
      if ((param.force_yes == FALSE) && (special == UNFORMAT))
        Confirm_Hard_Drive_Formatting(0); /* 0 means "unformat" - 0.91l */
      /* no confirmation requested for MIRROR ! */
    }

  /* "optimized" a lot in 0.91h, hopefully not too much... -ea */
  if ((!special) && (param.u==TRUE) && (param.q==FALSE)) /* full format / wipe? */
    {
    param.existing_format = FALSE; /* do not even check if /U non-/Q mode */
    }
  else
    {
    /* /Q /U clears metadata but does no format / wipe */
    /* /Q -- clears metadata after backing it up */
    /* -- -- works like /Q */
    /* CONCLUSION: We do not really have to worry about current  */
    /* for harddisks in /Q /U mode (uses DOS kernel default BPB) */

    if ( (param.drive_type!=FLOPPY) && (param.u==TRUE) )
      {
      /* no check for harddisk /U mode nor for /U /Q mode there. */
      param.existing_format = FALSE;
      }
    else /* it is a floppy --- or no /U flag given */
      {

      if (debug_prog==TRUE) 
        {
        if (special)
          {
            printf("[DEBUG]  Checking for existing format\n");
          }
        else
          {
            if (param.drive_type==FLOPPY)
              printf("[DEBUG]  Checking whether we need low-level format.\n");
            else
              printf("[DEBUG]  Checking whether UNFORMAT data can be saved.\n");
          }
        } /* debug_prog */
        
      /* Check to see if the media is currently formatted. */
      /* Needed for non-/U modes and for floppy disks, but in */
      /* /U non-/Q mode we already assume "not formatted" anyway. */
      param.existing_format = Check_For_Format(); /* trashes fat_type value! */
      /* no problem, done before media type / harddisk parameter setup */

      } /* check for existing format */
    } /* not FORMAT /U mode */


  Lock_Unlock_Drive(2);	/* lock drive (needed in Win9x / DOS 7.x) */
			/* SECOND lock, going lowlevel... - 0.91q */


  /* Determine and set media parameters */
  if (param.drive_type==FLOPPY)
    { /* <FLOPPY> */

    if (param.existing_format==TRUE)
      {
      found_format_sectors_per_track = sector_buffer[0x18];
      found_format_heads = sector_buffer[0x1a];
      }

    Set_Floppy_Media_Type(); /* configure hardware, set up BPB, fat_type FAT12 */

#if DETECT_FLOPPY_TWICE
    if ((param.existing_format == FALSE) && (param.u==FALSE))
      {
      /* try finding existing format again - after setting media type */
      if (debug_prog==TRUE)
        printf("[DEBUG]  Searching for existing format again...\n");

      Lock_Unlock_Drive(0);	/* re-enable filesystem access - 0.91q */

      param.existing_format = Check_For_Format(); /* trashes fat_type! No prob. */
      /* ... because it is done just before Set_Floppy_Media_Type */

      Lock_Unlock_Drive(2);	/* re-disable filesystem access - 0.91q */

      Set_Floppy_Media_Type(); /* configure hardware AGAIN, set up BPB, fat_type FAT12 */
      /* set media type again! - 0.91i */

      if (param.existing_format==TRUE)
        {
        /* depends on Check_For_Format to fill sector_buffer... */
	found_format_sectors_per_track = sector_buffer[0x18];
        found_format_heads = sector_buffer[0x1a];
        }
      } /* not /U and no existing format found */
#endif /* DETECT_FLOPPY_TWICE */

    /* if already formatted floppy, check whether geometry will change */
    if (param.existing_format == TRUE)
      {
      if ( ( parameter_block.bpb.sectors_per_cylinder !=
             found_format_sectors_per_track ) ||
           ( found_format_heads != parameter_block.bpb.number_of_heads ) )
        {
        printf("Will change size by formatting - forcing full format\n");
        printf("Old: %d sectors per track, %d heads. New: %d sect. %d heads\n",
          found_format_sectors_per_track, found_format_heads,
          parameter_block.bpb.sectors_per_cylinder,
          parameter_block.bpb.number_of_heads);
        param.existing_format = FALSE;
        }
      } /* existing format */
     /* if no existing format, we force full format anyway */

    } /* </FLOPPY> */
  else /* if harddisk: */
    { /* <HARDDISK> */

    /* 0.91m: the only place where "align" is used is here, in BPB setup!!! */
    Set_Hard_Drive_Media_Parameters(align); /* get default BPB etc., find FATxx fat_type */
    Enable_Disk_Access();

    } /* </HARDDISK> */
  /* *** Maybe we should have done drive setup earlier, for Check_...? *** */


  if (param.existing_format == FALSE) /* details changed in 0.91h */
    {
    if ( (param.drive_type==FLOPPY) &&
         ( (param.u!=TRUE) || (param.q!=FALSE) ) )
      printf("Cannot find existing format - forcing full format\n");
    if ( (param.drive_type!=FLOPPY) && (param.u==FALSE) )
      printf("Cannot find existing format - not saving UNFORMAT data.\n");
    }




  switch (special) /* 0.91l - special modes MIRROR and UNFORMAT */
    {
      case 0:		/* do nothing special */
        break;
      case MIRROR:	/* only update mirror data */
        printf("Writing a copy of the system sectors to the end of the drive:\n");
        printf("Boot sector, one FAT, root directory. Useful for UNFORMAT.\n");
#if 0	/* warning obsolete 0.91r */
//        printf("WARNING: This will OVERWRITE up to %s of possibly used space!\n",
//          (param.fat_type==FAT32) ? "ca. 16 MB" :
//            ( (param.drive_type==FLOPPY) ? "ca. 14 kB" : "ca. 192 kB" ) );
#endif
        Save_File_System(0);	/* includes filesystem usage stats display */
        		/* 0.91r: 0 is "abort if we would damage data" -ea */
        what_completed = WHAT_MIRROR; /* new 0.91p */
        goto format_complete;
        /* break; */
      case UNFORMAT:	/* only write back FAT/root/boot from mirror */
        printf("Overwriting boot sector, FATs and root directory with\n");
        printf("MIRROR/UNFORMAT data which you have saved earlier.\n");
        Restore_File_System(); /* restore is an euphemism for what it does! */
        what_completed = WHAT_UN; /* new 0.91p */
        goto format_complete;
        /* break; */
      default:
        printf("/Z:what???\n"); /* should never be reached */
    } /* switch special */


  /* ask for label if none given yet - 0.91r */
  if ((!param.v) && (!param.force_yes))
    {
    param.v = (Ask_Label(param.volume_label) != 0);
    /* printf("\n[%s]%s\n", param.volume_label, (param.v) ? "" : " (none)"); */
    } /* ask for label */


  /* Format Drive */
  if ( ( (param.existing_format==FALSE) && (param.drive_type==FLOPPY) ) ||
       ( (param.u==TRUE) && (param.q==FALSE) ) )
    {
    /* /U is Unconditional Format. */
    /* If floppy is unformatted or geometry changes, we must use this. */
    printf(" Full Formatting (wiping all data)\n");
    Unconditional_Format();
    Create_File_System();
    what_completed = WHAT_FORMAT; /* new 0.91p */
    goto format_complete;
    }

  if ( ( (param.existing_format==FALSE) && (param.drive_type!=FLOPPY) ) ||
       ( (param.u==TRUE) && (param.q==TRUE) ) ) /* changed in 0.91h */
    {
    /* /Q /U is Quick Unconditional format */
    /* Even unformatted harddisks do not need full "/U" format    */
    /* (harddisk /U format means "wipe all data, do surface scan) */
    /* (harddisk LOWLEVEL format is never done by this program!)  */
    printf(" QuickFormatting (only flushing metadata)\n");
    printf(" Warning: Resets bad cluster marks if any.\n");
    Create_File_System();
    what_completed = WHAT_QUICK; /* new 0.91p */
    goto format_complete;
    }

  if ( (param.u==FALSE) /* && (param.q==TRUE) */ )
    {

    /* this is the default, so if no /U given, it is irrelevant   */
    /* whether /Q is given or not... Should trigger FULL format   */
    /* if existing filesystem has other size or disk needs format */

    /* /Q is Safe Quick Format (checking for existing bad cluster list) */
    /* -- is Safe Quick Format (the same :-)) */
    printf(" Safe QuickFormatting (trying to save UnFormat data)\n");
    Save_File_System(1); /* side effect: checks for existing bad clust list */
    			/* 0.91r: 1 is "allow trashing of data clusters" -ea */
    			/* (unformat data is allowed to overwrite files)     */
    Create_File_System();
    what_completed = WHAT_SAFE; /* new 0.91p */
    goto format_complete;
    }

  format_complete:

#if 0
//  Force_Drive_Recheck(); /* must be delayed until drive unlocked - 0.91q */
#endif

  if ((bad_sector_map[0]>0) && (!special))
    Record_Bad_Clusters(); /* write list of bad clusters */
    /* may include copied list from previous filesystem - not in special modes! */

  switch (what_completed) /* new 0.91p - custom message */
    {
      case WHAT_FORMAT: printf("\nFormat"); break;
      case WHAT_QUICK: printf("\nQuickFormat"); break;
      case WHAT_SAFE: printf("\nSafe QuickFormat"); break;
      case WHAT_MIRROR: printf("\nMirror"); break;
      case WHAT_UN: printf("\nUnFormat"); break;
    } /* switch */
  printf(" complete.\n");

  Lock_Unlock_Drive(0);	/* unlock drive again (in Win9x / DOS 7.x) */
			/* release SECOND lock: enable filesystem  */
  Force_Drive_Recheck();		/* just in case... - 0.91q */
  Lock_Unlock_Drive(0);	/* unlock drive again (in Win9x / DOS 7.x) */
			/* release FIRST lock: allow full access!  */

  if ((param.s==TRUE) && (!special))
    {
      Write_System_Files();		/* call SYS */
      Force_Drive_Recheck();		/* just in case... - 0.91q */
    }

  if (!special) Display_Drive_Statistics();

  if ((param.drive_type==FLOPPY) && (param.force_yes==FALSE))
    {
      union REGS regs;

      /* printf("\nFormat another floppy (y/n)?\n"); */
      if (!special) /* or use what_completed switch / case with WHAT_...? */
          write(isatty(1) ? 1 : 2, "\nFormat", 7);
      else
          write(isatty(1) ? 1 : 2, "\nProcess", 8);
      write(isatty(1) ? 1 : 2, " another floppy (y/n)? ", 23);
      /* write to STDERR to keep message visible even if STDOUT redirected */

      /* Get keypress */
      regs.h.ah = 0x07;
      intdos(&regs, &regs);

      if (toupper(regs.h.al) == 'Y')
        {
        int bads;
        printf("\n%s next floppy...\n",
          special ? "Processing" : "Formatting");

	drive_statistics.bad_sectors = 0;
	bad_sector_map_pointer = 0;
	for (bads=0; bads < MAX_BAD_SECTORS; bads++)
	  {
	  bad_sector_map[bads] = 0;
	  }

        Lock_Unlock_Drive(1);	/* lock drive (needed in Win9x / DOS 7.x) */
				/* FIRST lock, file system still enabled. */
	/* (second level will be entered again when we really need it...) */

        param.v = FALSE;	/* force asking for new label for next disk */

        goto next_disk; /* *** LOOP AROUND FOR MULTIPLE FLOPPY DISKS *** */
        }
        printf("\n");
    } /* another floppy question, possibly jumping back */

  if (((debug_prog==TRUE) || (param.force_yes!=FALSE)) &&
    (bad_sector_map[0]>0) && (!special))
    exit(1);	/* 0.91v for Alain: errorlevel 1 for "has bad clusters" */
		/* (only if /z:seriously or /y or /d option active)     */

  exit(0);	/* not using Exit() here: no dual errorlevel, no unlock */

}

