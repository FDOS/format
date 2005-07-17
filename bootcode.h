/*
// Program:  Format
// Version:  0.91k (by Eric Auer 2004: FAT12+FAT16+FAT32)
// Written By:  Brian E. Reifsnyder
// Copyright:  2002-2004 under the terms of the GNU GPL, Version 2
// Module Name:  bootcode.h
// Module Description:  Bootcode header file.
*/


/* The 0.91k code prints the string at "return address from call PRN" */
/* which makes the code work at any position inside a boot sector :-) */
unsigned char boot_code[] = {
'\x31', '\xc0',	/* xor ax,ax	*/
'\xfa',		/* cli		*/
'\x8e', '\xd0',	/* mov ss,ax	*/
'\xbc', '\0', '\x7c',	/* mov sp,0x7c00	*/
'\xfb',		/* sti		*/
'\x0e', '\x1f',	/* push cs, pop ds */
'\xeb', '\x19',	/* jmp XSTRING	*/

'\x5e',		/* PRN: pop si	; string offs	*/
'\xfc',		/* cld		*/
'\xac',		/* XLOOP: lodsb	*/
'\x08', '\xc0',	/* or al,al	; EOF reached?	*/
'\x74', '\x09',	/* jz EOF	*/
'\xb4', '\x0e',	/* mov ah,0x0e	; tty function	*/
'\xbb', '\x07', '\0',	/* mov bx,7		*/
'\xcd', '\x10',	/* int 0x10	; VIDEO BIOS	*/
'\xeb', '\xf2',	/* jmp short XLOOP		*/

'\x31', '\xc0',	/* EOF: xor ax,ax ; getkey	*/
'\xcd', '\x16',	/* int 0x16	; KEYB BIOS	*/
'\xcd', '\x19',	/* int 0x19	; REBOOT	*/
'\xf4',		/* HANG: hlt	; save energy	*/
'\xeb', '\xfd',	/* jmp short HANG		*/

'\xe8', '\xe4', '\xff'	/* XSTRING: call PRN	*/
}; /* boot message follows */

char boot_message[] =
  "This is not a bootable disk. Please insert a bootable floppy and\r\n"
  "press any key to try again...\r\n\0";
