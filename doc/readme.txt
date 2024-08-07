README.TXT

Program:     FORMAT

Version:     0.91 series

Author:      Brian E. Reifsnyder (1999-2003)
             reifsnyderb@mindspring.com

Maintainer:

Description: Prepares a disk with a file system. See the help.txt file
             for a list of the many features in this version.

Note:        After formatting a hard disk you MAY have to reboot the
             PC for the file system to be properly recognized.

Licensing:   This program is licensed under the GNU GPL.

Warranties:  This program does not have any warranties, either stated
             or implied.  By using this program you are assuming full 
             responsibility for the program's execution. (This can
             nuke your pre-existing OS, so be careful.)

Operating    Systems this program is known to work under:
             FreeDOS ("normal" and for DOSEMU image drives), MS-DOS 6.22

Windows:     Windows NT / 2000 / XP / 2003 (.NET) will not allow any DOS
             program to format an harddisk partition. Windows 95 / 98 /
             ME should work (if you LOCK the drive). From Windows XP /
             ME on, FATs > 16 MB each and 64k cluster size are allowed.
             Win XP will however refuse to FORMAT > 32 GB FAT itself, to
             promote the use of NTFS (which FreeDOS does NOT support!).

             Actually, I think Windows' FAT 32 GB limit is due to
             real mode memory limitations and slow speed (maybe).
             But 32 GB is quite a lot of space for DOS!
