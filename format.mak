# This makefile is for format and Watcom C / OpenWatcom (2005)
# Minimum CFLAGS: -ms

CC=wcc
CLINK=wcl
CFLAGS=-w -0 -ms -fpc -zp1 -q
# -w warnall -0 8086 compat -fpc floating point library calls (no FPU)
# -ms small memory model -zp1 byte-align structures
LDFLAGS=
LDLIBS=
RM=rm -f
OBJS1=createfs.obj floppy.obj hdisk.obj main.obj savefs.obj bcread.obj prf.obj
OBJS2=userint.obj driveio.obj getopt.obj init.obj recordbc.obj uformat.obj

# build targets:

all: format.exe

format.exe: $(OBJS1) $(OBJS2)
	$(CLINK) $(CFLAGS) $(LDFLAGS) *.obj $(LDLIBS) -fe=format.exe

.c.obj: .AUTODEPEND
	$(CC) $(CFLAGS) $*.c

# clean up:

clean: .SYMBOLIC
	$(RM) *.obj

clobber: .SYMBOLIC
	$(RM) *.bak
	$(RM) *.dsk
	$(RM) *.exe
	$(RM) *.obj
	$(RM) *.swp

