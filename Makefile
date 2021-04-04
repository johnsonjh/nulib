#
# UNIX Makefile for NuLib v3.2
#

# To make a smaller executable, you can exclude the Binary II routines
# by setting CFLAGS= -DNO_BLU
# To exclude the UNIX compression routines, add -DNO_UCOMP

# Under UTS 2.1, use -eft to make the linker use the EFT library.
#EFT=-eft

# Select appropriate flag... -g for debugging, -O for optimized.
#CFLAGS=-g $(EFT)
CFLAGS=-O -fcommon $(EFT)
#CFLAGS=-p $(EFT)

HDRS=nudefs.h nuread.h nuview.h nuadd.h nuext.h nupdel.h nupak.h nuetc.h \
  nublu.h nucomp.h nucompfn.h
SRCS=numain.c nuread.c nuview.c nuadd.c nuext.c nupdel.c nupak.c nuetc.c \
  nublu.c nucomp.c nushk.c nusq.c 
OBJS=numain.o nuread.o nuview.o nuadd.o nuext.o nupdel.o nupak.o nuetc.o \
  nublu.o nucomp.o nushk.o nusq.o
ARCFILES=README NOTES Makefile make.apw linker.scr linked.scr \
  mkshk nulib.mak nulib.lnk *.h *.c

LIBS=
#LIBS= -lx		# For XENIX/386 users
CC=gcc

all:	nulib

nulib: ${OBJS}
	${CC} ${CFLAGS} ${OBJS} -o nulib ${LIBS} 

#
# .o targets
#

numain.o: numain.c nudefs.h nuread.h nuview.h nuadd.h nuext.h nupdel.h nublu.h\
  nuetc.h

nuread.o: nuread.c nudefs.h nuread.h nupak.h nuetc.h crc.h

nuview.o: nuview.c nudefs.h nuview.h nuread.h nuetc.h

nuadd.o:  nuadd.c  nudefs.h nuadd.h nuread.h nuadd.h nupak.h nuetc.h

nuext.o:  nuext.c  nudefs.h nuext.h nuread.h nuext.h nupak.h nuetc.h

nupdel.o: nupdel.c nudefs.h nupdel.h nuread.h nuadd.h nupak.h nupdel.h nuetc.h

nupak.o:  nupak.c  nudefs.h nupak.h nuetc.h nucomp.h nucompfn.h

nublu.o:  nublu.c  nudefs.h nublu.h nuview.h nuetc.h

nushk.o:  nushk.c  nudefs.h nupak.h

nusq.o:   nusq.c   nudefs.h nupak.h

nuetc.o:  nuetc.c  nudefs.h nuetc.h

nucomp.o:	nucomp.c nudefs.h nucomp.h nucompfn.h nuetc.h

#
# other targets
#

saber:
	#load $(CFLAGS) $(SRCS) $(LIBS)

# shar version 3.49
#	-c : add "cut here" line at top
#	-o : base name for output files
#	-l48 : max size is 48KB, but don't split files
#	-v : (not used) turn off verbose msgs
shar:
	shar349 -c -osh.files/nulib -l48 $(ARCFILES)

tar:
	tar cvf nulib.tar $(ARCFILES) nulib.doc

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f nulib

