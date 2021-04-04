/*
 * nuread.c - read NuFX archives (header info only) into structures
 *
 * NuLib v3.2  March 1992  Freeware (distribute, don't sell)
 * By Andy McFadden (fadden@uts.amdahl.com)
 */
#ifdef APW
segment "NuMain"
#endif

#include "nudefs.h"
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#ifdef UNIX
# include <sys/types.h>
#endif

#ifdef MSDOS     /* For file IO */
# include <stdlib.h>		/* errno, among others */
# include <string.h>
# include <io.h>
# include <sys/types.h>
# include <sys/stat.h>
#endif

#ifdef CRC_TAB
# include "crc.h"     /* fast CRC lookup */
#endif
#include "nuread.h"
#include "nupak.h"  /* uses PAKBUFSIZ */
#include "nuetc.h"

#define UNKNOWN_FN	"<No Filename>"

/* quick proc to save x00 bytes of static storage */
void
OtherArc(str1, str2)
char *str1, *str2;
{
    fprintf(stderr, "File may be %s; try \"%s\".\n", str1, str2);
}

/* swap two bytes if HiLo is TRUE */
void
HiSwap(ptr, a, b)
onebyt *ptr;
register onebyt a, b;
{
    register onebyt tmp;

    if (HiLo) {
	tmp = ptr[a], ptr[a] = ptr[b], ptr[b] = tmp;
    }
}


/* copy bytes from buffer to buffer, reversing byte order if necessary */
void
BCopy(srcptr, destptr, num, order)
register onebyt *srcptr, *destptr;
int num;
BOOLEAN order;	/* true if byte ordering is important */
{
    register int i = num--;

    if (order && HiLo) {
	while (i--) {  /* copy & reverse */
	    *(destptr+i) = *(srcptr + num - i);  /* dest+3 = src + 3 - 3 .. */
	}
    } else if (order) {
	while (i--) {  /* copy only */
	    *(destptr+i) = *(srcptr + i);
	}
    } else {
	while (i--) {  /* byte ordering not important; just copy */
	    *(destptr+i) = *(srcptr+i);
	}
    }
}


/*
 * Calculate CRC on a region
 *
 * A CRC is the result of a mathematical operation based on the
 * coefficients of a polynomial when multiplied by X^16 then divided by
 * the generator polynomial (X^16 + X^12 + X^5 + 1) using modulo two
 * arithmetic.
 *
 * This routine is a slightly modified verison of one found in:
 * _Advanced Programming Techniques for the Apple //gs Toolbox_
 * By Morgan Davis and Dan Gookin (Compute! Publications, Inc.)
 * It can either calculate the CRC bit-by-bit or use a table.
 * [ one of the few //gs books worth the money	+atm ]
 */
twobyt
CalcCRC(seed, ptr, count)
twobyt seed;  /* initial value for CRC */
register onebyt *ptr;  /* pointer to start of data buffer */
register int count;    /* number of bytes to scan through - note 64K max */
{
#ifndef CRC_TAB
    register int x;
#endif
    register twobyt CRC = seed;

    do {
#ifndef CRC_TAB
	CRC ^= *ptr++ << 8;		  /* XOR hi-byte of CRC w/data	 */
	for (x = 8; x; --x)		  /* Then, for 8 bit shifts...	 */
	    if (CRC & 0x8000)		  /* Test hi order bit of CRC	 */
		CRC = CRC << 1 ^ 0x1021;  /* if set, shift & XOR w/$1021 */
	    else
		CRC <<= 1;		  /* Else, just shift left once. */
#else
	CRC = updcrc(*ptr++, CRC);	  /* look up new value in table  */
#endif
    } while (--count);
    return (CRC);
}


/*
 * Test an archive's integrity.
 *
 * Reads the entire file, and checks CRCs for certain things.
 */
void
NuTest(filename, options)
char *filename;
char *options;
{
    ListHdr *archive;
    onebyt *copybuf;  /* buffer for reading record */
    int partial;   /* size for partial read */
    unsigned int rec;
    RNode *RNodePtr;
    MHblock *MHptr;
    TNode *TNodePtr;
    long hdr_size, total_size, thread_size;
    int srcfd;	/* file descriptor */
    int thread;
    twobyt CRC=0, RecordCRC;
    long CRCsum = 0L;	/* sum of CRCs for all records */
    BOOLEAN check_thread_crc;	/* TRUE if we want to check a give thread */
    static char *procName = "NuTest";

    printf("Testing %s", filename);
    if (verbose) printf("\n");
    else       { printf("...");  fflush(stdout); }

    archive = NuRead(filename);  /* this catches most errors... */

    MHptr = archive->MHptr;
    RNodePtr = archive->RNodePtr;
    copybuf = (onebyt *) Malloc(PAKBUFSIZ);
    if ((srcfd = open(filename, O_RDONLY | O_BINARY)) < 0)
	Fatal("Unable to close archive", procName);
    if (lseek(srcfd, (off_t) MHsize, S_ABS) < 0)  /* seek past master block */
	Fatal("Bad seek (MH)", procName);

    for (rec = 0; rec < (unsigned int) MHptr->total_records; rec++) {
	if (verbose) printf("Record %d (%s): ", rec, RNodePtr->filename);
	hdr_size = (long) RNodePtr->RHptr->attrib_count;
	hdr_size += (long) RNodePtr->filename_length;
	total_size = hdr_size;
	TNodePtr = RNodePtr->TNodePtr;
	for (thread=0; thread < (int)RNodePtr->RHptr->total_threads; thread++){
	    if (TNodePtr == (TNode *) NULL) {
		fprintf(stderr, "Internal error: Bad thread structure\n");
		Quit(-1);
	    }
	    hdr_size += (long) THsize;
	    total_size += (long) THsize;
	    total_size += TNodePtr->THptr->comp_thread_eof;
	    TNodePtr = TNodePtr->TNext;
	}
	if (verbose) {
	    printf("total record size = %ld (%d threads)\n", total_size,
		(int) RNodePtr->RHptr->total_threads);
	    fflush(stdout);
	}

	/* read record header */
	RecordCRC = 0;
	while (hdr_size != 0L) {
	    if (hdr_size > (long) PAKBUFSIZ) {
		partial = (unsigned int) PAKBUFSIZ;
		hdr_size -= (long) PAKBUFSIZ;
	    } else {
		partial = (unsigned int) hdr_size;
		hdr_size = 0L;
	    }

	    if (read(srcfd, copybuf, partial) != partial) {
		fprintf(stderr, ">>> Read error");
		if (verbose) fprintf(stderr, "\n");
		else fprintf(stderr,
			" - record %d (%s)\n",  rec, RNodePtr->filename);
		fprintf(stderr, "Operation aborted.\n");
		Quit(-1);
	    }
	    if (verbose) RecordCRC = CalcCRC(CRC, (onebyt *) copybuf, partial);
	}

	TNodePtr = RNodePtr->TNodePtr;
	for (thread=0; thread < (int)RNodePtr->RHptr->total_threads; thread++){
	    if (lseek(srcfd, (off_t) TNodePtr->fileposn, S_ABS) < 0)
		Fatal("whoops!", procName);
	    thread_size = TNodePtr->THptr->comp_thread_eof;

	    /* decide whether or not to check thread CRCs */
	    check_thread_crc = FALSE;
	    if (RNodePtr->RHptr->version_number >= 2)	/* valid CRCs */
		if (TNodePtr->THptr->thread_class == 2)	/* data_thread */
		    check_thread_crc = TRUE;
	    if (RNodePtr->RHptr->version_number == 3)	/* CRC of uncom data */
		if (TNodePtr->THptr->thread_format != 0x0000)
		    check_thread_crc = FALSE;		/* can't check comp */

	    if (check_thread_crc) CRC = 0xffff;
	    while (thread_size != 0L) {
		if (thread_size > (long) PAKBUFSIZ) {
		    partial = (unsigned int) PAKBUFSIZ;
		    thread_size -= (long) PAKBUFSIZ;
		} else {
		    partial = (unsigned int) thread_size;
		    thread_size = 0L;
		}

		if (read(srcfd, copybuf, partial) != partial) {
		    fprintf(stderr, ">>> Read error in thread");
		    if (verbose) fprintf(stderr, " %d\n", thread);
		    else fprintf(stderr, " - record %d (%s), thread %d\n",
			    rec, RNodePtr->filename, thread);
		    fprintf(stderr, "Operation aborted.\n");
		    Quit(-1);
		}

		if (verbose)
		    RecordCRC = CalcCRC(RecordCRC, (onebyt *)copybuf, partial);

		/* calculate CRC for thread, and compare with thread_crc */
		if (check_thread_crc)
		    CRC = CalcCRC(CRC, (onebyt *) copybuf, partial);
#ifdef DEBUG
		printf(
"At posn %ld: rec %d/thread %d (%ld bytes)  CalcCRC = 0x%.4x (0x%.4x)\n",
TNodePtr->fileposn, rec, thread, thread_size, CRC, TNodePtr->THptr->thread_crc
		);
#endif
	    }

	    /* check and see if CRC matches */
	    if (check_thread_crc) {
		if (CRC != TNodePtr->THptr->thread_crc) {
		    fprintf(stderr, ">>> Bad CRC for thread %d",
			thread);
		    if (verbose) fprintf(stderr, "\n");
		    else fprintf(stderr, " in record %d\n", rec);
		} else {
		    if (verbose) printf("--- CRC matched for thread %d\n",
			thread);
		}
	    }
	    TNodePtr = TNodePtr->TNext;
	}

	if (verbose) {
	    printf("--- CRC for entire record was $%.4x\n", RecordCRC);
	    CRCsum += RecordCRC;
	}
	RNodePtr = RNodePtr->RNext;
    }
    if (close(srcfd) < 0)
	Fatal("Unable to close archive", procName);

    free(copybuf);
    if (verbose) printf("Sum of CRCs = $%.8lx\n", CRCsum);
    printf("done.\n");
}


/*
 * Read thread header data, and skip data fields
 */
static TNode *
ReadThreads(fd, RHptr, RNodePtr, CRC_ptr)
int fd;
RHblock *RHptr;
RNode *RNodePtr;
twobyt *CRC_ptr;  /* CRC seed; result is returned thru this */
{
    int i;
    unsigned int size;
    BOOLEAN first;
    TNode *TNodePtr, *THeadPtr = (TNode *) NULL;
    THblock *THptr;
    char filebuf[THsize];
    twobyt CRC = *CRC_ptr;
    static char *procName = "ReadThreads";

    RNodePtr->unc_len = 0L;
    RNodePtr->comp_len = 0L;
    first = TRUE;
    for (i = 0; i < RHptr->total_threads; i++) {
	if (first) {  /* create first block, or... */
	    TNodePtr = (TNode *) Malloc(sizeof(TNode));
	    THeadPtr = TNodePtr;
	    first = FALSE;
	} else {  /* create next block and go on */
	    TNodePtr->TNext = (TNode *) Malloc(sizeof(TNode));
	    TNodePtr = TNodePtr->TNext;
	}
	TNodePtr->TNext = (TNode *) NULL;

	/* Create the thread header block, and read it in */
	TNodePtr->THptr = (THblock *) Malloc(sizeof(THblock));
	THptr = TNodePtr->THptr;

	if (size = read(fd, filebuf, THsize) < THsize) {  /* should be 16 */
	    printf("read size = %d, THsize = %d\n", size, THsize);
	    Fatal("ReadThread (THblock)", procName);
	}
	CRC = CalcCRC(CRC, (onebyt *) filebuf, 16);  /* hdr CRC part(s) 5/5 */

	/* copy all fields... */
	BCopy(filebuf+0, (onebyt *) &THptr->thread_class, 2, TRUE);
	BCopy(filebuf+2, (onebyt *) &THptr->thread_format, 2, TRUE);
	BCopy(filebuf+4, (onebyt *) &THptr->thread_kind, 2, TRUE);
	BCopy(filebuf+6, (onebyt *) &THptr->thread_crc, 2, TRUE);
	BCopy(filebuf+8, (onebyt *) &THptr->thread_eof, 4, TRUE);
	BCopy(filebuf+12, (onebyt *) &THptr->comp_thread_eof, 4, TRUE);

	RNodePtr->unc_len += THptr->thread_eof;
	RNodePtr->comp_len += THptr->comp_thread_eof;
	if (THptr->comp_thread_eof > 2097152L)	/* SANITY CHECK */
	    fprintf(stderr, "Sanity check: found comp_thread_eof > 2MB\n");
    }

    /* skip the actual data */
    TNodePtr = THeadPtr;
    for (i = 0; i < RHptr->total_threads; i++) {
	THptr = TNodePtr->THptr;
	if ((TNodePtr->fileposn = lseek(fd, (off_t) 0, S_REL)) < 0)
	    Fatal("Bad thread posn lseek()", procName);

	/* pull filenames out of threads, if present */
	if (THptr->thread_class == 0x0003) {	/* filename thread */
	    RNodePtr->filename = (char *) Malloc(THptr->thread_eof +1);
	    if (read(fd, RNodePtr->filename, THptr->thread_eof) < 0) {
		fprintf(stderr, "Error on thread %d\n", i);
		Fatal("Unable to read filename", procName);
	    }
	    RNodePtr->filename[THptr->thread_eof] = '\0';
	    RNodePtr->real_fn_length = THptr->thread_eof;

	    {		/* patch to fix bug in ShrinkIt v3.0.0 */
		int j, name_len = strlen(RNodePtr->filename);

		for (j = 0; j < name_len; j++) {
		    RNodePtr->filename[j] &= 0x7f;	/* clear hi bit */
		}
	    }

	    if (lseek(fd, (off_t) TNodePtr->fileposn, S_ABS) < 0)
		Fatal("Unable to seek back after fn", procName);
	}
	if (lseek(fd, (off_t) THptr->comp_thread_eof, S_REL) < 0)
	    Fatal("Bad skip-thread seek", procName);
	TNodePtr = TNodePtr->TNext;
    }
    *CRC_ptr = CRC;
    return (THeadPtr);
}


/*
 * Read header data from a NuFX archive into memory
 */
ListHdr *
NuRead(filename)
char *filename;
{
    int fd;  /* archive file descriptor */
    char namebuf[MAXFILENAME];
    int rec, num;
    BOOLEAN first;
    twobyt namelen;
    twobyt CRC;
    ListHdr *ListPtr;  /* List Header struct */
    MHblock *MHptr;  /* Master Header block */
    RNode *RNodePtr;  /* Record Node */
    RHblock *RHptr;  /* Record Header block */
    onebyt filebuf[RECBUFSIZ];	/* must be > RH, MH, or atts-RH size */
    static char *procName = "NuRead";
    char *cp;

    ListPtr = (ListHdr *) Malloc(sizeof(ListHdr));  /* create head of list */
    ListPtr->arc_name = (char *) Malloc(strlen(filename)+1); /* archive fnam */
    strcpy(ListPtr->arc_name, filename);
    ListPtr->MHptr = (MHblock *) Malloc(sizeof(MHblock));  /* master block */

    if ((fd = open(filename, O_RDONLY | O_BINARY)) < 0) {
	if (errno == ENOENT) {
	    fprintf(stderr, "%s: can't find file '%s'\n", prgName, filename);
	    Quit (-1);
	} else
	    Fatal("Unable to open archive", procName);
    }

    /* create and read the master header block */
    MHptr = ListPtr->MHptr;
    if (read(fd, filebuf, MHsize) < MHsize) {
	fprintf(stderr, "File '%s' may not be a NuFX archive\n", filename);
	Fatal("Unable to read Master Header Block", procName);
    }

    CRC = CalcCRC(0, (onebyt *) filebuf+8, MHsize-8);  /* master header CRC */

    /* Copy data to structs, correcting byte ordering if necessary */
    BCopy(filebuf+0, (onebyt *) MHptr->ID, 6, FALSE);
    BCopy(filebuf+6, (onebyt *) &MHptr->master_crc, 2, TRUE);
    BCopy(filebuf+8, (onebyt *) &MHptr->total_records, 4, TRUE);
    BCopy(filebuf+12, (onebyt *) &MHptr->arc_create_when, sizeof(Time), FALSE);
    BCopy(filebuf+20, (onebyt *) &MHptr->arc_mod_when, sizeof(Time), FALSE);
    BCopy(filebuf+28, (onebyt *) &MHptr->master_version, 2, TRUE);
    BCopy(filebuf+30, (onebyt *) MHptr->reserved1, 8, FALSE);
    BCopy(filebuf+38, (onebyt *) &MHptr->master_eof, 4, TRUE);  /* m_v $0001 */
    BCopy(filebuf+42, (onebyt *) MHptr->reserved2, 6, FALSE);

    if (strncmp(MHptr->ID, MasterID, 6)) {
	fprintf(stderr, "\nFile '%s' is not a NuFX archive\n", filename);
	if ((filebuf[0] == 10) && (filebuf[1] == 71) &&
	    (filebuf[2] == 76) && (filebuf[18] == 2))
#ifdef NO_BLU
	    OtherArc("Binary II", "unblu");
#else
	    fprintf(stderr, "File may be Binary II; try 'B' option\n");
#endif
	if ((filebuf[0] == '\037') && (filebuf[1] == '\036'))
	    OtherArc("packed", "unpack");
	if ((filebuf[0] == (onebyt)'\377') && (filebuf[1] == '\037'))
	    OtherArc("compacted", "uncompact");
	if ((filebuf[0] == '\037') && (filebuf[1] == (onebyt)'\235'))
	    OtherArc("compressed", "uncompress");
	if ((filebuf[0] == 0x76) && (filebuf[1] == 0xff))
	    OtherArc("SQueezed", "usq");
	if ((filebuf[0] == 0x04) && (filebuf[1] == 0x03) &&
	    (filebuf[2] == 0x4b) && (filebuf[3] == 0x50))
	    OtherArc("a ZIP archive", "UnZip");
	if (!strncmp((char *) filebuf, "ZOO", 3))	/* zoo */
	    OtherArc("a ZOO archive", "zoo");
	if ((filebuf[0] == 0x1a) && (filebuf[1] == 0x08))	/* arc */
	    OtherArc("an ARC archive", "arc");
	if (!strncmp((char *) filebuf, "SIT!", 4))	/* StuffIt */
	    OtherArc("a StuffIt archive", "StuffIt (Macintosh)");
	if (!strncmp((char *) filebuf, "<ar>", 4))	/* system V arch */
	    OtherArc("a library archive (Sys V)", "ar");
	if (!strncmp((char *) filebuf, "!<arch>", 7))
	    OtherArc("a library archive", "ar");
	if (!strncmp((char *) filebuf, "#! /bin/sh", 10) ||
	    !strncmp((char *) filebuf, "#!/bin/sh", 9))
	    OtherArc("a shar archive", "/bin/sh");
	if (!strncmp((char *) filebuf, "GIF87a", 6))
	    OtherArc("a GIF picture", "?!?");
	/* still need ZIP */

	Quit (-1);
    }

    if (CRC != MHptr->master_crc)
	printf("WARNING: Master Header block may be corrupted (bad CRC)\n");

    if (MHptr->master_version > MAXMVERS)
	printf("WARNING: unknown Master Header version, trying to continue\n");

    /* main record read loop */
    first = TRUE;
    for (rec = 0; rec < (unsigned int) MHptr->total_records; rec++) {
	if (first) {  /* allocate first, or... */
	    ListPtr->RNodePtr = (RNode *) Malloc(sizeof(RNode));
	    RNodePtr = ListPtr->RNodePtr;
	    first = FALSE;
	} else {  /* allocate next, and go on */
	    RNodePtr->RNext = (RNode *) Malloc(sizeof(RNode)); /* next Rnode */
	    RNodePtr = RNodePtr->RNext;  /* move on to next record */
	}
	RNodePtr->RNext = (RNode *) NULL;

	RNodePtr->RHptr = (RHblock *) Malloc(sizeof(RHblock)); /* alloc blk */
	/* expansion here */
	RHptr = RNodePtr->RHptr;
	if (read(fd, filebuf, RHsize) < RHsize) {  /* get known stuff */
	    fprintf(stderr,"%s: error in record %d (at EOF?)\n", prgName, rec);
	    Fatal("Bad RHblock read", procName);
	}

	/* rec hdr CRC part 1/5 */
	CRC = CalcCRC(0, (onebyt *) filebuf+6, RHsize-6);

	BCopy(filebuf+0, (onebyt *) RHptr->ID, 4, FALSE);
	BCopy(filebuf+4, (onebyt *) &RHptr->header_crc, 2, TRUE);
	BCopy(filebuf+6, (onebyt *) &RHptr->attrib_count, 2, TRUE);
	BCopy(filebuf+8, (onebyt *) &RHptr->version_number, 2, TRUE);
	BCopy(filebuf+10, (onebyt *) &RHptr->total_threads, 2, TRUE);
	BCopy(filebuf+12, (onebyt *) &RHptr->reserved1, 2, TRUE);
	BCopy(filebuf+14, (onebyt *) &RHptr->file_sys_id, 2, TRUE);
	BCopy(filebuf+16, (onebyt *) &RHptr->file_sys_info, 1, TRUE);
	BCopy(filebuf+17, (onebyt *) &RHptr->reserved2, 1, TRUE);
	BCopy(filebuf+18, (onebyt *) &RHptr->access, 4, TRUE);
	BCopy(filebuf+22, (onebyt *) &RHptr->file_type, 4, TRUE);
	BCopy(filebuf+26, (onebyt *) &RHptr->extra_type, 4, TRUE);
	BCopy(filebuf+30, (onebyt *) &RHptr->storage_type, 2, TRUE);
	BCopy(filebuf+32, (onebyt *) &RHptr->create_when, sizeof(Time), FALSE);
	BCopy(filebuf+40, (onebyt *) &RHptr->mod_when, sizeof(Time), FALSE);
	BCopy(filebuf+48, (onebyt *) &RHptr->archive_when, sizeof(Time), FALSE);
	BCopy(filebuf+56, (onebyt *) &RHptr->option_size, 2, TRUE);
	/* expansion here */

	if (strncmp(RHptr->ID, RecordID, 4)) {
	    fprintf(stderr, "%s: Found bad record ID (#%d) -- exiting\n",
								prgName, rec);
	    Quit (-1);
	}

	/* read remaining (unknown) attributes into buffer, if any */
	num = RHptr->attrib_count - RHsize - 2;
	if (num > RECBUFSIZ) {
	    fprintf(stderr, "ERROR: attrib_count > RECBUFSIZ\n");
	    Quit (-1);
	}
	if (num > 0) {
	    if (read(fd, filebuf, num) < num)
		Fatal("Bad xtra attr read", procName);
	    CRC = CalcCRC(CRC, (onebyt *) filebuf, num);  /* hdr CRC part 2/5 */
	}

	if (read(fd, (char *) &namelen, 2) < 2)  /* read filename len */
	    Fatal("Bad namelen read", procName);
	CRC = CalcCRC(CRC, (onebyt *) &namelen, 2);  /* rec hdr CRC part 3/5 */

	HiSwap((onebyt *) &namelen, 0, 1);
	/* read filename, and store in struct */
	if (namelen > MAXFILENAME) {
	    fprintf(stderr, "ERROR: namelen > MAXFILENAME\n");
	    Quit (-1);
	}
	RNodePtr->filename_length = namelen;

	if (namelen > 0) {
	    RNodePtr->real_fn_length = namelen;
	    if (read(fd, namebuf, namelen) < namelen)
		Fatal("Bad namebuf read", procName);
	    /* rec hdr CRC part 4/5 */
	    CRC = CalcCRC(CRC, (onebyt *) namebuf, namelen); 

	    RNodePtr->filename = (char *) Malloc(namelen+1);  /* store fname */
	    BCopy(namebuf, (onebyt *) RNodePtr->filename, namelen, FALSE);
	    RNodePtr->filename[namelen] = '\0';
	    for (cp = RNodePtr->filename; *cp; cp++)
		*cp &= 0x7f;		/*strip high bits from old-style names*/
	} else {
	    RNodePtr->filename = UNKNOWN_FN;
	    RNodePtr->real_fn_length = strlen(UNKNOWN_FN);
	}

	RNodePtr->TNodePtr = ReadThreads(fd, RHptr, RNodePtr, &CRC);
	/* rec hdr CRC part 5/5 calculated by ReadThreads */

	if (CRC != RHptr->header_crc) {
	    printf("WARNING: Detected a bad record header CRC\n");
	    printf("         Rec %d in file '%.60s'\n",rec,RNodePtr->filename);
	}
    }

    /* begin adding new files at  this point */
    if ((ListPtr->nextposn = lseek(fd, (off_t) 0, S_REL)) < 0)
	Fatal("Bad final lseek()", procName);

    if (close(fd) < 0) {
	Fatal("Bad close", procName);
    }

    if (MHptr->master_version > 0x0000) {
	if ((fourbyt) MHptr->master_eof != (fourbyt) ListPtr->nextposn) {
	    printf("WARNING: master_eof (stored)=%ld, nextposn (actual)=%ld\n",
		MHptr->master_eof, ListPtr->nextposn);
	    printf(
		"         (master_eof will be fixed if archive is changed)\n");
	}
    }
	
    return (ListPtr);
}

