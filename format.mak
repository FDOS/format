# This makefile is for format and Watcom C / OpenWatcom (2005)
# Minimum CFLAGS: -ms

CC=wcc
CLINK=wcl
CFLAGS=-w -0 -ms -fpc -zp1
# -w warnall -0 8086 compat -fpc floating point library calls (no FPU)
# -ms small memory model -zp1 byte-align structures
LDFLAGS=
LDLIBS=
RM=command /c del
OBJS1=createfs.obj floppy.obj hdisk.obj main.obj savefs.obj bcread.obj prf.obj
OBJS2=userint.obj driveio.obj getopt.obj init.obj recordbc.obj uformat.obj

# build targets:

all: format.exe

format.exe: $(OBJS1) $(OBJS2)
	$(CLINK) $(CFLAGS) $(LDFLAGS) *.obj $(LDLIBS) -fe=format.exe

# compile targets:

# very convenient but not available in Turbo C 2.01 - generic C/OBJ rule:
# .c.obj:
#	$(CC) $(CFLAGS) $*.c

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

