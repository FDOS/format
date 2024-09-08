# Asterisk for supporting long cmdlines in raw DOS (via env. var.)
# (only supported by certain OW tools). Run "whelp tools" for more.
CC=wcc
CLINK=*wcl

LDFLAGS=
LDLIBS=

UPX=upx
UPXFLAGS=-qq --ultra-brute --8086

CFLAGS=-wx -0 -mc -fpc -zp1

# -wx  warnall
# -0   8086 compat
# due to size of message catalogs, large data is needed if compile with kitten
# -ms  small memory model / -mc compact memory model for large data
# -fpc floating point library calls (no FPU)
# -zp1 byte-align structures

OBJS=createfs.obj floppy.obj hdisk.obj main.obj   &
     savefs.obj bcread.obj prf.obj userint.obj    &
     driveio.obj getopt.obj init.obj recordbc.obj &
     uformat.obj msghlpr.obj kitten.obj

pack: format.exe .SYMBOLIC
  $(UPX) $(UPXFLAGS) $<

format.exe: $(OBJS)
  $(CLINK) $(CFLAGS) $(LDFLAGS) $< $(LDLIBS) -fe=$@

.c.obj: kitten.h
  $(CC) $(CFLAGS) $*.c

# rm is built-in to wmake
clean: .SYMBOLIC
  rm *.obj

clobber: .SYMBOLIC
  rm -f *.bak *.dsk *.exe *.obj *.swp *.map *.err
