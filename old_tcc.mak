# This makefile is for format and Borland Turbo C++ 3.0.
# Changed to use line-initial TAB rather than space: Makes it
# Borland Turbo C 2.01 compatible. Also using no generic rule
# anymore and splitting OBJS into 2 variables, for del command
# line. Plus made command /c explicit for using del!
# Do not forget that all Turbo C must be in PATH and that you
# must have TURBOC.CFG in the current directory.
# Update 2005: link with prf.c light drop-in printf replacement.
#
# Minimum CFLAGS: -ms
# Smaller for Turbo C: -M -O -N -Z -w -a- -f- -ms

CC=tcc
CLINK=tcc
CFLAGS=-M -N -ln -w -a- -f- -f87- -ms -r- -c
# -M linkmap -O jump optimize -N stack check -f87- no fpu code
# -w warnall -a- no word align -f- no fpu emulator
# -ms small memory model -ln no default libs linked ...
# -r register variables -k standard stack frame ...
LDFLAGS=-M -N -ln -w -a- -f- -f87- -ms -r-
LDLIBS=
RM=command /c del
OBJS1=createfs.obj floppy.obj hdisk.obj main.obj savefs.obj bcread.obj prf.obj
OBJS2=userint.obj driveio.obj getopt.obj init.obj recordbc.obj uformat.obj

# build targets:

all: format.exe

format.exe: $(OBJS1) $(OBJS2)
	$(CLINK) $(LDFLAGS) -eformat *.obj $(LDLIBS) 

# compile targets:

# very convenient but not available in Turbo C 2.01 - generic C/OBJ rule:
# .c.obj:
#	$(CC) $(CFLAGS) -c $*.c

createfs.obj:
	$(CC) $(CFLAGS) createfs.c

floppy.obj:
	$(CC) $(CFLAGS) floppy.c

hdisk.obj:
	$(CC) $(CFLAGS) hdisk.c

main.obj:
	$(CC) $(CFLAGS) main.c

savefs.obj:
	$(CC) $(CFLAGS) savefs.c

userint.obj:
	$(CC) $(CFLAGS) userint.c

driveio.obj:
	$(CC) $(CFLAGS) driveio.c

getopt.obj:
	$(CC) $(CFLAGS) getopt.c

init.obj:
	$(CC) $(CFLAGS) init.c

recordbc.obj:
	$(CC) $(CFLAGS) recordbc.c

uformat.obj:
	$(CC) $(CFLAGS) uformat.c

bcread.obj:
	$(CC) $(CFLAGS) bcread.c

prf.obj:
	$(CC) $(CFLAGS) prf.c


# clean up:

clean:
	$(RM) *.obj

clobber: 
	$(RM) *.bak
	$(RM) *.dsk
	$(RM) *.exe
	$(RM) *.obj
	$(RM) *.swp

