# This makefile is for format and Borland Turbo C++ 3.0.
# Changed to use line-initial TAB rather than space: Makes it
# Borland Turbo C 2.01 compatible. Also using no generic rule
# anymore and splitting OBJS into 2 variables, for del command
# line. Plus made command /c explicit for using del!
# Do not forget that all Turbo C must be in PATH and that you
# must have TURBOC.CFG in the current directory.
# Update 2005: link with prf.c light drop-in printf replacement.
#
# Minimum CFLAGS: -mc  (can use -ms if not using kitten)
# Smaller for Turbo C: -M -O -N -Z -w -a- -f- -mc
MMODEL=c

CC=tcc
CLINK=tcc
CFLAGS=-M -N -ln -w -a- -f- -f87- -m$(MMODEL) -r- -c
# -M linkmap -O jump optimize -N stack check -f87- no fpu code
# -w warnall -a- no word align -f- no fpu emulator
# -ms small memory model 
# -mc compact memory model (should be used with kitten)
# -ln no default libs linked ...
# -r register variables 
# -k standard stack frame ...
LDFLAGS=-M -N -ln -w -a- -f- -f87- -m$(MMODEL) -r-
LDLIBS=
RM=command /c del
OBJS1=createfs.obj floppy.obj hdisk.obj main.obj savefs.obj bcread.obj prf.obj
OBJS2=userint.obj driveio.obj getopt.obj init.obj recordbc.obj uformat.obj
OBJS3=msghlpr.obj kitten.obj

# build targets:

all: format.exe

format.exe: $(OBJS1) $(OBJS2) $(OBJS3)
	$(CLINK) $(LDFLAGS) -eformat *.obj $(LDLIBS) 

# compile targets:

# very convenient but not available in Turbo C 2.01 - generic C/OBJ rule:
# .c.obj:
#	$(CC) $(CFLAGS) -c $*.c

createfs.obj: kitten.h
	$(CC) $(CFLAGS) createfs.c

floppy.obj: kitten.h
	$(CC) $(CFLAGS) floppy.c

hdisk.obj: kitten.h
	$(CC) $(CFLAGS) hdisk.c

main.obj: kitten.h
	$(CC) $(CFLAGS) main.c

savefs.obj: kitten.h
	$(CC) $(CFLAGS) savefs.c

userint.obj: kitten.h
	$(CC) $(CFLAGS) userint.c

driveio.obj: kitten.h
	$(CC) $(CFLAGS) driveio.c

getopt.obj:
	$(CC) $(CFLAGS) getopt.c

init.obj: kitten.h
	$(CC) $(CFLAGS) init.c

recordbc.obj: kitten.h
	$(CC) $(CFLAGS) recordbc.c

uformat.obj: kitten.h
	$(CC) $(CFLAGS) uformat.c

bcread.obj: kitten.h
	$(CC) $(CFLAGS) bcread.c

prf.obj:
	$(CC) $(CFLAGS) prf.c

kitten.obj: kitten.h
	$(CC) $(CFLAGS) kitten.c

msghlpr.obj: kitten.h
	$(CC) $(CFLAGS) msghlpr.c


# clean up:

clean:
	$(RM) *.obj

clobber: 
	$(RM) *.bak
	$(RM) *.dsk
	$(RM) *.exe
	$(RM) *.obj
	$(RM) *.swp

