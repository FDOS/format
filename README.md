FreeDOS FORMAT
==============


Simplified syntax (only common standard options):
-------------------------------------------------

* FORMAT drive: [/V[:label]] [/Q] [/U] [/F:size] [/S] [/D]
* FORMAT drive: [/V[:label]] [/Q] [/U] [/T:tracks /N:sectors] [/S] [/D]
* FORMAT drive: [/V[:label]] [/Q] [/U] [/4] [/S] [/D]
 

 /V:label   Specifies a volume label for the disk, stores date and time of it.

 /S         Calls SYS to make the disk bootable and to add system files.

 /D         Be very verbose and show debugging output. For bug reports.

 /Q         Quick formats the disk. If not combined with /U, can be UNFORMATed
            and bad cluster list is preserved (not preserved in /Q /U mode).

 /U         Unconditionally formats the disk. Lowlevel format if floppy disk.

 /F:size    Specifies the size of the floppy disk to format. Normal sizes are:
            360, 720, 1200, 1440, or 2880 (unit: kiloBytes). /F:0 shows a list.

 /4         Formats a 360k floppy disk in a 1.2 MB floppy drive. As 1.2 MB
            uses narrower tracks, format can be too weak for 360k drives.

 /T:tracks  Specifies the number of tracks on a floppy disk.

 /N:sectors Specifies the number of sectors on a floppy disk.


