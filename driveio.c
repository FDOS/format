/*
// Program:  Format
// Version:  0.91v
// (0.90b/c/d - warnings fixed, buffers far, boot sector IO - EA 2003)
// (0.91b..j / k... - more fixes - Eric Auer 2003 / 2004)
// (0.91q Locking more Win9x compatible - EA)
// (0.91u allow "get access flag" error code 0x11 for DW ramdisk - EA 2005)
// (0.91v "physical lock: access denied": warn only if /d - EA for Alain 2006)
// Written By:  Brian E. Reifsnyder
// Copyright:  2002-2006 under the terms of the GNU GPL, Version 2
// Module Name:  driveio.c
// Module Description:  Functions specific to accessing a disk.
*/

#define DIO

#include "driveio.h"
#include "format.h"
#include "userint.h" /* Critical_* */

unsigned int FloppyBootReadWrite(char, int, ULONG,  void far *, unsigned);



/* Clear Huge Sector Buffer */
void Clear_Huge_Sector_Buffer(void)
{
  memset(huge_sector_buffer_0, 0, sizeof(huge_sector_buffer_0));
  memset(huge_sector_buffer_1, 0, sizeof(huge_sector_buffer_1));
}



/* Clear Sector Buffer */
void Clear_Sector_Buffer(void)
{
  memset(sector_buffer_0, 0, sizeof(sector_buffer_0));
  memset(sector_buffer_1, 0, sizeof(sector_buffer_1));
}


/* exit program, after unlocking drives - introduced 0.91r  */
/* supports "normal" (MS) and "verbose" (FD) errorlevels    */
/* MS: 0 ok, 3 user aborted, 4 fatal error, 5 not confirmed */
void Exit(int mserror, int fderror)
{
  if ((mserror==3) || (mserror==4)) {	/* aborted by user / fatal error? */
    Lock_Unlock_Drive(0);		/* remove inner lock */
    Lock_Unlock_Drive(0);		/* remove outer lock */
    /* if there were less locks than unlocks, no problem: checked below. */
  }
  if (debug_prog)
    exit(fderror);	/* use the "verbose errorlevel" */
  if (mserror==4)	/* "fatal error"? */
    printf(" [Error %d]\n", fderror);
  exit(mserror);	/* use the "normal errorlevel" */
} /* Exit */


/* 0.91l - Win9x / DOS 7+: LOCK (lock TRUE) / UNLOCK (lock FALSE) drive */
/* changed 0.91q: lock can be 2 for the nested second level 0 lock...   */
void Lock_Unlock_Drive(int lock)
{
  static int lock_nesting = 0;
  /* 0.91q: As recommended by http://msdn.microsoft.com "Level 0 Lock":   */
  /* get level 0 lock, use filesystem functions to prepare (if needed),   */
  /* get level 0 lock AGAIN, this time with permissions 4, use sector I/O */
  /* and IOCTL functions or int 13 to create filesystem, then release the */
  /* inner level 0 lock, use filesystem functions for cleanup, release    */
  /* the outer level 0 lock, done. You must NOT drop the locks or exit at */
  /* a moment when the filesystem is in inconsistent / nonFAT state, or   */
  /* Win9x will assume that the filesystem is still as before the lock... */

#if 0	/* insert a WORKING drive existence check here */
  if (lock) {		/* drive existance test */
      /* problem: int 21.36 causes a critical error in FreeDOS 2035  */
      /* (should return error code for "non FAT filesystem" instead) */
      /* ... so we need something else as magic drive test :-( ...   */
      regs.x.ax = 0x3600;			/* get drive usage stats */
      regs.x.dx = param.drive_number + 1;	/* A: is 1 etc. */
      regs.x.cx = 0x5555;
      intdos(&regs, &regs);			/* call int 0x21 DOS API */
      if (regs.x.ax == 0xffff)			/* sectors per cluster, -1 is nonfat */
          goto locking_invalid_drive;
      if (regs.x.cx != 512)	{		/* bytes per sector, we only do 512 */
          printf("Not 512 bytes per sector.\n");
          goto locking_invalid_drive;
      }
  } /* check for drive existence (only at LOCK, not at UNLOCK) */
#endif /* drive existence check would be nice but not working yet - 0.91r */
/* for now: LOCK as drive exist check lucklily DOES work in FAT32 kernels */
/* (unformatted drives are reported as existing, just what we need...)    */
/* LOCK and Get_Device_Parameters both return error 0x0f for completely   */
/* non-existing drives / okay for unformatted... G.D.P. good for FATxx :) */
/* ===> For now, we use G.D.P., LOCK and BootSecRead as drive and file-   */
/* system detectors. Note: if no filesystem, B.S.R. *should* return 0x07, */
/* unknown media, but in FreeDOS 2035 it is 1 / 0x0c (1x/32), yuck! <===  */
/* The PROBLEM: IF DOS 6-, "invalid drive" detected only AFTER confirm... */

  /* *** 0.91l try to LOCK the drive (logical volume) *** */
  if (_osmajor < 7) /* dos.h: Major DOS version number. 7 is "Win9x" */
      return;		/* ignore the whole locking on older DOS versions */

  if ((lock_nesting==0) && (!lock)) {
/*
 *    if (debug_prog==TRUE)
 *        printf("[DEBUG]  Extraneous drive unlock attempt ignored.\n");
 */
      return;
  }

  if (debug_prog==TRUE) printf("[DEBUG]  DOS 7+, %sing drive%s\n",
      lock ? ( (lock>1) ? "FORMAT-LOCK" : "LOCK" )
             : "UNLOCK",
      lock ? "" : " (by one level)"); /* (for UNLOCK, BH/DX are irrelevant) */

  regs.x.cx = lock ? 0x084a : 0x086a; /* (un)LOCK logical FAT1x volume */
  do
      {
          regs.x.ax = 0x440d; /* IOCTL */
          if ((!lock) && (param.fat_type == FAT32)) regs.h.ch = 0x48; /* FAT32 */
          regs.h.bh = 0;	/* big fat lock, not one of the "levels" 1..3 */
          regs.h.bl = param.drive_number + 1; /* A: is 1 etc. */
          regs.x.dx = (lock>1) ? 4 : 0;	/* when getting SECOND level 0 lock */
          			/* ... then select "format" permissions! */
          regs.x.cflag = 0;	/* (2 is block new file mappings) */
          intdos(&regs, &regs); /* call int 0x21 DOS API */
          regs.h.ch += 0x40;    /* try FAT32 (etc.) mode next */
          if (!lock) regs.h.ch = 0x77; /* give up if earlier if unlocking */
      } while ( (regs.h.ch <= 0x48) && regs.x.cflag );

  if (regs.x.cflag && lock) /* ignore unlock errors */
      {
          unsigned int lockerrno = regs.x.ax;

          if (lockerrno == 0x0f) /* invalid drive */
            {
              locking_invalid_drive:
              printf(" Invalid drive! Aborting.\n");
              Exit(4,29);
            }

          regs.x.ax = 0x3000; /* get full DOS version and OEM ID */
          intdos(&regs, &regs);
          if (regs.h.bh == 0xfd)
              printf(" FreeDOS l. lock error 0x%x ignored.\n",lockerrno);
          else
            {
              printf(" Could not lock logical drive (error 0x%x)! Aborting.\n",
                lockerrno);
              Exit(4,25);
            }
      } /* LOCK error */

      /* for int 13 writes you have to lock the physical volume, too */
  if ((param.drive_type == FLOPPY) && (lock!=1)) /* 0.91q: not for outer level */
      {
          regs.x.ax = 0x440d;  /* IOCTL */
          regs.x.cx = lock ? 0x084b : 0x086b; /* (un)LOCK physical FAT1x volume */
          if ((!lock) && (param.fat_type == FAT32)) regs.h.ch = 0x48; /* FAT32 */
          regs.h.bh = 0;       /* lock level (or use 3?) (DX / BL still set) */
          /* or should BL be the BIOS driver number now (MSDN)? RBIL: 01=a: */
          regs.x.cflag = 0;
          intdos(&regs, &regs); /* call int 0x21 DOS API */
          if (regs.x.cflag && lock) /* ignore unlock errors */
            {
              unsigned int lockerrno = regs.x.ax;

              regs.x.ax = 0x3000; /* get full DOS version and OEM ID */
              intdos(&regs, &regs);
              if (debug_prog==TRUE)	/* error 5, "access denied", is "normal", */
                {			/* so limit warning to /d mode - 0.91v    */
                  if (regs.h.bh == 0xfd)
                      printf(" FreeDOS p. lock error 0x%x ignored.\n", lockerrno);
                  else
                      printf(" Could not lock physical floppy drive (error 0x%x)!?\n",
                        lockerrno);
                }
                  /* could { ... exit(1); } here, but MSDN does not even */
                  /* suggest getting a physical drive lock at all..!? */
            } /* LOCK error */
      } /* extra FLOPPY treatment */

  lock_nesting += (lock) ? 1 : -1;	/* maintain lock nesting count */
}   /* *** drive is now LOCKed if lock, else UNLOCKed *** */



/* changed 0.91k: if number of sectors is negative, do not abort on fail */
int Drive_IO(int command,unsigned long sector_number,int number_of_sectors)
{
  unsigned int return_code;
  int error_counter = 0;
  int allowfail = (number_of_sectors < 0);
  if (allowfail) number_of_sectors =  -number_of_sectors;
  if (!number_of_sectors)
    {
      printf("Drive_IO(x,y,0)?\n");
      return -1;
    }

  retry:

  if (param.fat_type != FAT32)
    {
    return_code =
       TE_AbsReadWrite(param.drive_number,number_of_sectors,sector_number,
	   (number_of_sectors==1) ? sector_buffer : huge_sector_buffer,
	   command);
    }
  else
    {
    return_code =
       FAT32_AbsReadWrite(param.drive_number,number_of_sectors,sector_number,
	   (number_of_sectors==1) ? sector_buffer : huge_sector_buffer,
	   command);
    /* return code: lo is int 24 / DI code, hi is bitmask: */
    /* 1 bad command 2 bad address mark 3 (int 26 only) write protection */
    /* 4 sector not found 8 dma failure 10 crc error 20 controller fail  */
    /* 40 seek fail 80 timeout, no response */
    }

  if ( (return_code==0x0207 /* 0x0408 */) &&
    ((param.drive_number & 0x80)==0) /* avoid fishy retry */
    ) /* changed -ea */
    {
    /* As per RBIL, if 0x0408 is returned, retry with high bit of AL set. */
    /* MY copy of RBIL mentions 0x0207 for that case!? -ea */
    param.drive_number = param.drive_number + 0x80; /* set high bit. */
    error_counter++;
/*
 *  if (debug_prog==TRUE) printf("[DEBUG]  Retrying with AL |= 0x80.\n");
 */
    if (debug_prog==TRUE) printf("+"); /* give some hint about the change -ea */
    goto retry;
    }

  if ( (return_code!=0) && (error_counter<3) )
    {
    error_counter++;
/*
 *  if (debug_prog==TRUE) printf("[DEBUG]  Retry number %2d\n",error_counter);
 */
    if (debug_prog==TRUE) printf("*"); /* give some problem indicator -ea */
    goto retry;
    }

  if (return_code!=0)
    {
    if ( ((param.drive_number & 0x7f)>=2) ||
         (command==WRITE) || (sector_number!=0L)  )
      {
      if (allowfail)
        {
        if (debug_prog==TRUE)
          printf("* bad sector(s): %ld (code 0x%x) on %s *\n",
            sector_number, return_code, (command==WRITE) ? "WRITE" : "READ");
        else
          printf("#");
        }
      else
        {
        /* Added more details -ea */
        printf("Drive_IO(%s %ld, count %d ) [%s] [drive %c%c]\n",
          (command==WRITE) ? "WRITE" : "READ", sector_number, number_of_sectors,
          (param.fat_type==FAT32) ? "FAT32" : "FAT12/16",
          'A' + (param.drive_number & 0x7f),
          (param.drive_number & 0x80) ? '*' : ':' );
        Critical_Error_Handler(DOS, return_code);
        }
      }
    else
      {
      /* Only HARDDISK and WRITE and READ-beyond-BOOTSECTOR errors   */
      /* are CRITICAL, others are handled by the CALLER instead! -ea */
      printf("#");
      }
    }

  param.drive_number = param.drive_number & 0x7f; /* unset high bit. */
  return (return_code);
}



/* Do not confuse this with DOS 7.0+ drive locking */
void Enable_Disk_Access(void) /* DOS 4.0+ drive access flag / locking */
{
  unsigned char category_code;
  unsigned long error_code=0;

  category_code = (param.fat_type == FAT32) ? 0x48 : 0x08;

  /* Get the device parameters for the logical drive */

  regs.h.ah=0x44;                     /* IOCTL Block Device Request      */
  regs.h.al=0x0d;                     /* IOCTL */
  regs.h.bl=param.drive_number + 1;
  regs.h.ch=category_code;            /* 0x08 if !FAT32, 0x48 if FAT32   */
  regs.h.cl=0x67;                     /* Get Access Flags                */
  regs.x.dx=FP_OFF(&access_flags);	/* only 2 bytes: 0 and the flag */
  sregs.ds =FP_SEG(&access_flags);
  access_flags.special_function = 0;

  intdosx(&regs, &regs, &sregs);

  error_code = regs.h.al;

  if (regs.x.cflag)
    {
    /* BO: if invalid function: try to format anyway maybe access
	   flags do not work this way in this DOS (e.g. DRDOS 7.03) */
    /* error code 0x11: "not same device" - returned by DW ramdisk? */
    if (error_code == 0x1 || error_code == 0x16 || error_code == 0x11 )
      {
        if (debug_prog==TRUE) printf("[DEBUG]  No GENIOCTL/x867 access flags (code 0x%02x)?\n",
          error_code);
	return;
      }

    /* Add error trapping here */
    printf("\nCannot get access flags (error %02x). Aborting.\n",
      error_code);
    Exit(4,26);
    }

  if (access_flags.disk_access==0) /* access not yet enabled? */
    {
    access_flags.disk_access++;
    access_flags.special_function = 0;

    regs.h.ah = 0x44;                     /* IOCTL Block Device Request          */
    regs.h.al = 0x0d;                     /* IOCTL */
    regs.h.bl = param.drive_number + 1;
    regs.h.ch = category_code;            /* 0x08 if !FAT32, 0x48 if FAT32       */
    regs.h.cl = 0x47;                     /* Set access flags                    */
    regs.x.dx = FP_OFF(&access_flags);
    sregs.ds  = FP_SEG(&access_flags);
    intdosx( &regs, &regs, &sregs);

    error_code = regs.h.al;

    if (regs.x.cflag)
	{
      /* Add error trapping here */
	printf("\nCannot enable access flags (error %02x). Aborting.\n",
          error_code);
	Exit(4,27);
	}
    }

  if (debug_prog==TRUE) printf("[DEBUG]  Enabled access flags.\n");

}



/* FAT32_AbsReadWrite() is modified from TE_AbsReadWrite(). */
unsigned int FAT32_AbsReadWrite(char DosDrive, int count, ULONG sector,
  void far * buffer, unsigned ReadOrWrite)
{
    unsigned diskReadPacket_seg;
    unsigned diskReadPacket_off;

    void far * diskReadPacket_p;

    struct {
	unsigned long  sectorNumber;
	unsigned short count;
	void far *address;
	} diskReadPacket;
    union REGS regs;

    struct {
	unsigned direction  : 1 ; /* low bit */
	unsigned reserved_1 : 12;
	unsigned write_type : 2 ;
	unsigned reserved_2 : 1 ;
	} mode_flags;

    diskReadPacket.sectorNumber = sector;
    diskReadPacket.count        = count;
    diskReadPacket.address      = buffer;

    diskReadPacket_p =& diskReadPacket;

    diskReadPacket_seg = FP_SEG(diskReadPacket_p);
    diskReadPacket_off = FP_OFF(diskReadPacket_p);

    mode_flags.reserved_1 = 0;
    mode_flags.write_type = 0;
    mode_flags.direction  = (ReadOrWrite == READ) ? 0 : 1;
    mode_flags.reserved_2 = 0;

    DosDrive++;

    /* no inline asm for Turbo C 2.01! -ea */
    /* Turbo C also complains about packed bitfield structures -ea */
    {
      struct SREGS s;
      regs.x.ax = 0x7305;
      regs.x.bx = diskReadPacket_off;
      s.ds = diskReadPacket_seg;
      regs.x.cx = 0xffff;
      regs.h.dl = DosDrive;
      regs.x.si = mode_flags.direction; /* | (mode_flags.write_type << 13); */
      intdosx(&regs, &regs, &s);
      return (regs.x.cflag ? regs.x.ax : 0);
    }
    
}

/* TE_AbsReadWrite() is written by Tom Ehlert. */
unsigned int TE_AbsReadWrite(char DosDrive, int count, ULONG sector,
  void far * buffer, unsigned ReadOrWrite)
{
    struct {
	unsigned long  sectorNumber;
	unsigned short count;
	void far *address;
	} diskReadPacket;
    void far * dRP_p;
    union REGS regs;
    struct SREGS sregs;


    if ( (count==1) && (sector==0) && ((DosDrive & 0x7f) < 2) )
      {
      /* ONLY use lowlevel function if only a floppy boot sector is     */
      /* affected: maybe DOS simply does not know filesystem params yet */
      /* the lowlevel function also tells DOS about the new fs. params! */
#if 0
//      if (debug_prog==TRUE)
//        printf("[DEBUG]  Using lowlevel boot sector access...\n");
#endif
        regs.x.ax = FloppyBootReadWrite(DosDrive & 0x7f, 1, 0,
          buffer, ReadOrWrite); /* (F.B.R.W. by -ea) */

        if ((debug_prog==TRUE) && (regs.h.al != 0))
#if 0
//          printf("[DEBUG] INT 13 status:   0x%02x\n", regs.h.al);
#else
            printf("[%02x]", regs.h.al);
#endif

        switch (regs.h.al) { /* translate BIOS error code to DOS code */
                           /* AL is the AH that int 13h returned... */
          case 0: return 0; /* no error */
          case 3: return 0x0300; /* write protected */
          case 0x80: return 0x0002; /* drive not ready */
          case 0x0c: return 0x0007; /* unknown media type */
          case 0x04: return 0x000b; /* read fault */
          default: return 0x0808; /* everything else: "sector not found" */
          /* 02 - address mark not found, 07 - drive param error */
        } /* case */
        /* return 0x1414; *//* never reached */
      }

    diskReadPacket.sectorNumber = sector;
    diskReadPacket.count        = count;
    diskReadPacket.address      = buffer;

    dRP_p = &diskReadPacket;

    regs.h.al = DosDrive; /* 0 is A:, 1 is B:, ... */
    /* in "int" 25/26, the high bit of AL is an extra flag */
    regs.x.bx = FP_OFF(dRP_p);
    sregs.ds = FP_SEG(dRP_p);
    regs.x.cx = 0xffff;

    switch(ReadOrWrite)
	{
	    case READ:  int86x(0x25,&regs,&regs,&sregs); break;
	    case WRITE: int86x(0x26,&regs,&regs,&sregs); break;
	    default:
		printf("TE_AbsR...: mode %02x\n", ReadOrWrite);
		Exit(4,28);
	 }

    
    return regs.x.cflag ? regs.x.ax : 0;
}

/* TE_* not usable while boot sector invalid, so... */
unsigned int FloppyBootReadWrite(char DosDrive, int count, ULONG sector,
  void far * buffer, unsigned ReadOrWrite)
{

    union REGS regs;
    struct SREGS sregs;
    int i = 0;
    int theerror = 0;

    if ((DosDrive > 1) || ((DosDrive & 0x80) != 0))
      return 0x0808; /* sector not found (DOS) */
    if ((sector != 0) || (count != 1))
      return 0x0808; /* sector not found (DOS) */

    do
      {
      if (ReadOrWrite == WRITE)
        {
        regs.x.ax = 0x0301; /* read 1 sector */
        }
      else
        {
        regs.x.ax = 0x0201; /* write 1 sector */
        }
      regs.x.bx = FP_OFF(buffer);
      regs.x.cx = 1; /* 1st sector, 1th (0th) track */
      regs.h.dl = DosDrive; /* drive ... */
      regs.h.dh = 0; /* 1st (0th) side */
      sregs.es = FP_SEG(buffer);
      regs.x.cflag = 0;
      int86x(0x13,&regs,&regs,&sregs);
      
      if (regs.x.cflag && (regs.h.ah != 0))	/* carry check added 0.91s */
        {
        i++; /* error count */
        theerror = regs.h.ah;   /* BIOS error code */
        if (theerror == 6)
          i--; /* disk change does not count as failed attempt */
        }
      else
        {

#if 0	/* better do this where Drive_IO is CALLED, not implicitly! */
//        regs.h.ah = 0x0d;
//        intdos(&regs,&regs); /* flush buffers, reset disk system */
//
//        regs.h.ah = 0x32;
//        regs.h.dl = DosDrive + 1; /* 0 default 1 A: 2 B: ... */
//        intdosx(&regs,&regs,&sregs); /* force DOS to re-read boot sector */
//        /* we ignore the returned DS:BX pointer to the new DPB (if AL=0) */
//        segread(&sregs); 	/* restore defaults */
#endif

        return 0; /* WORKED */
        }
      }
      while (i < 3); /* do ... while */ /* 3 attempts are enough */
        /* (you do not want to use floppy disk with weak boot sector) */

    return theerror; /* return ERROR code */
    /* fixed in 0.91b: theerror / AH, not AX is the code! */
}

