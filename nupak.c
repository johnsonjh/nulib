/*
 * nupak.c - interface to the compression routines
 *
 * NuLib v3.2  March 1992  Freeware (distribute, don't sell)
 * By Andy McFadden (fadden@uts.amdahl.com)
 */
#define TRY_II
#ifdef APW
segment "Compress"
#endif

#include "nudefs.h"
#include "stdio.h"
/*#include <fcntl.h>*/	/* "nucomp.h" includes <fcntl.h> for us */
#include "nuread.h"	/* need THblock */
#include "nucomp.h"	/* includes "nucompfn.h" + "types.h" */

#ifdef MSDOS  /* for file I/O */
# include <io.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <errno.h>
#endif

#include "nupak.h"
#include "nuetc.h"

#define CONVSIZ	1024

long packedSize;  /* global - size of file after packing */
onebyt lastseen;  /* used in crlf(); must be set by caller */


/*
 * Make a little spinning thing.
 *
 * This just lets the user know that the whole thing hasn't stalled on him.
 * Prints a character, then backspaces so that the next thing will overwrite
 * it.
 *
 * Currently called by FCopy(), unpak_SHK(), pak_SHK()
 */
void
Spin()
{
    static char *sp = "/-\\|";
    static int posn = 0;

    posn++;
    if ((posn < 0) || (posn > 3))
	posn = 0;
    putchar(sp[posn]);
    putchar('\b');
    fflush(stdout);
}


/*
 * Convert the end-of-line terminator between systems.
 *
 * Compile-time defines determine the value that things are translated to;
 * the value of "translate" determines what they are translated from.  This
 * will write the contents of the buffer to the passed file descriptor,
 * altering bytes as necessary.  Max buffer size is 64K.  Note that the
 * syntax is the same as for write();
 *
 * This would have been a lot easier without IBM...  lastseen is the last
 * character seen (used only in CRLF translations).  This needs to be set
 * by the caller (FCopy(), extract_files()).
 *
 * The putc_ncr() procedure in nusq.c does its own processing; this was
 * somewhat unavoidable.
 *
 * BUGS: This proc will have to be re-written.  It would be nice to be
 * able to pack files with a CRLF translation, not just unpack... but you
 * can't just do buffer writes for that.  It'll take some work, and will
 * probably appear in the next version.
 */
unsigned int
crlf(dstfd, buffer, length)
int dstfd;
onebyt *buffer;
unsigned int length;
{
    register BOOLEAN doconv;
    register onebyt *bptr = buffer;
    register unsigned int idx;
    static char *procName = "crlf";
    unsigned int partial;   /* size for partial read/write */
    onebyt tobuf[2048];
    onebyt *toptr;
    int conv;
    unsigned int origlength = length;

    if ((transfrom == -1) && (transto == -1)) {  /* no translation necessary */
	return (write(dstfd, buffer, length));
    }
    if (transfrom < -1 || transfrom > 2) {
	fprintf(stderr, "%s: unknown translation type %d\n",
							prgName, transfrom);
	fprintf(stderr, "%s: assuming conversion 0 (from CR)\n", prgName);
	transfrom = 0;
    }
    if (transto < -1 || transto > 2) {
	fprintf(stderr, "%s: unknown translation type %d\n",
							prgName, transto);
	fprintf(stderr, "%s: assuming conversion 0 (to CR)\n", prgName);
	transto = 0;
    }

    /* macro defs for system-dependent actions */
#ifdef UNIX
# define DEFCONVFROM	if (*bptr == 0x0a)  /* LF */ \
			    doconv = TRUE
#  define DEFCONVTO	*(toptr++) = 0x0a
#else
# ifdef APW
#  define DEFCONVFROM	if (*bptr == 0x0d)  /* CR */ \
			    doconv = TRUE
#  define DEFCONVTO	*(toptr++) = 0x0d
# endif
# ifdef MSDOS
#  define DEFCONVFROM	if ((*bptr == 0x0a) && (lastseen == 0x0d)) { \
			    doconv = TRUE; \
			    toptr--;  /*already put CR; back up over it*/ \
			} \
			lastseen = *bptr
#  define DEFCONVTO	*(toptr++) = 0x0d; \
			*(toptr++) = 0x0a
# endif
# ifndef APW
# ifndef MSDOS
#  define DEFCONVFROM	if (*bptr == 0x0a)  /* LF */ \
			    doconv = TRUE
# endif /* none2 */
# endif /* none1 */
#endif /* UNIX */

    while (length != 0) {
	if (length > CONVSIZ) {
	    partial = CONVSIZ;
	    length -= CONVSIZ;
	} else {
	    partial = length;
	    length = 0;
	}

	/* uses an explicit flag rather than "continue" for clarity... */
	toptr = tobuf;
	for (idx = partial; idx > 0; idx--, bptr++) {
	    doconv = FALSE;
	    switch (transfrom) {
	    case -1:  /* convert from current system's terminator */
		DEFCONVFROM;
		break;
	    case 0:
		if (*bptr == 0x0d)  /* CR */
		    doconv = TRUE;
		break;
	    case 1:
		if (*bptr == 0x0a)  /* LF */
		    doconv = TRUE;
		break;
	    case 2:
		if ((*bptr == 0x0a) && (lastseen == 0x0d)) {
		    doconv = TRUE;
		    toptr--;  /*already outputed CR; back up over it*/
		}
		lastseen = *bptr;
		break;
	    }


	    if (doconv) {
		switch (transto) {
		case -1:  /* convert to current system's terminator */
		    DEFCONVTO;
		    break;
		case 0:
		    *(toptr++) = 0x0d;
		    break;
		case 1:
		    *(toptr++) = 0x0a;
		    break;
		case 2:
		    *(toptr++) = 0x0d;
		    *(toptr++) = 0x0a;
		    break;
		}
	    } else {
		*(toptr++) = *bptr;
	    }
	} /* for loop */
		    
	if (write(dstfd, tobuf, (toptr-tobuf)) != (toptr-tobuf))
	    Fatal("Dest write failed", procName);
    }  /* while loop */
    return (origlength);
}


/*
 * Read a file, and place in another file at current posn.  We can't read more
 * than PAKBUFSIZ at a time, so for some files it will have to be broken down
 * into pieces.  Note PAKBUFSIZ is expected to be an int (defined in nupak.h),
 * and can't be any larger than read() can handle (64K... unsigned 16-bit int).
 *
 * The transl option is present for NuUpdate and NuDelete, which have to
 * copy old records to a new archive w/o performing translation.
 */
void
FCopy(srcfd, dstfd, length, copybuf, transl)
int srcfd;    /* source file descriptor (must be open & seek()ed) */
int dstfd;    /* destination file descriptor (must be open & seek()ed) */
fourbyt length; /* number of bytes to copy */
onebyt *copybuf;
BOOLEAN transl;  /* maybe do text translation? */
{
    unsigned int partial;   /* size for partial read/write */
    static char *procName = "FCopy";

    if (transl) lastseen = '\0';
    while (length != 0L) {
	if (length > (long) PAKBUFSIZ) {
	    partial = (unsigned int) PAKBUFSIZ;
	    length -= (long) PAKBUFSIZ;
	    if (verbose) Spin();
	} else {
	    partial = (unsigned int) length;
	    length = 0L;
	}

	if (read(srcfd, copybuf, partial) != partial)
	    Fatal("Source read failed", procName);
	if (transl) {  /* do text translation if user wants it */
	    if (crlf(dstfd, copybuf, partial) != partial)
		Fatal("Dest write failed (c)", procName);
	} else {  /* NEVER do translation */
	    if (write(dstfd, copybuf, partial) != partial)
		Fatal("Dest write failed (w)", procName);
	}
    }
}


/*
 * Add a range of bytes from one file into another, packing them.
 *
 * Set up stuff, then call the appropriate pack routine.  Returns the actual
 * algorithm used (thread_format), since the compression algorithm could
 * fail, storing the file in uncompressed format instead.  The packed length
 * is stored in a global variable.
 *
 * Since we're only using version 0 records, we don't need to propagate the
 * thread_crc.
 *
 * Compression routines must do the following:
 * - compress data from one file descriptor to another, given two seeked
 *   file descriptors and a length value.  They may not rely on EOF conditions
 *   for either file.
 * - return the packing method actually used.  If they cope with failure
 *   by starting over with something different, the successful method should
 *   be returned.  Failure may be handled in the switch statement below.
 */
twobyt
PackFile(srcfd, dstfd, thread_eof, thread_format, buffer)
int srcfd;    /* source file descriptor (must be open & seek()ed) */
int dstfd;    /* destination file descriptor (must be open & seek()ed) */
fourbyt thread_eof; /* size of input */
int thread_format; /* how to pack the bytes */
onebyt *buffer; /* alloc in main prog so we don't have to each time */
{
    long length = (long) thread_eof;
    twobyt retval = thread_format;  /* default = successful pack */
    long srcposn, dstposn;
    static char *procName = "PackFile";

    switch (thread_format) {
    case 0x0000:  /* uncompressed */
	if (verbose) { printf("storing...", thread_format);  fflush(stdout); }
	FCopy(srcfd, dstfd, length, buffer, TRUE);
	packedSize = length;
	break;

    case 0x0001:  /* SQUeeze */
	if (verbose) {
	    printf("[can't squeeze; storing]...");
	    fflush(stdout);
	} else {
	    printf("WARNING: can't squeeze; files stored uncompressed\n");
	}
	FCopy(srcfd, dstfd, length, buffer, TRUE);
	packedSize = length;
	retval = 0x0000;  /* uncompressed */
	break;

    case 0x0002:  /* LZW (ShrinkIt) */
	if (verbose) { printf("shrinking...");  fflush(stdout); }
	/* packedSize set by pak_SHK */
	retval = pak_SHK(srcfd, dstfd, length, buffer);
	break;
    case 0x0003:  /* LZW II (ShrinkIt) */
	if (verbose) {
	    printf("[can't do LZW II; storing]...");
	    fflush(stdout);
	} else {
	    printf("WARNING: can't do LZW II; files stored uncompressed\n");
	}
	FCopy(srcfd, dstfd, length, buffer, TRUE);
	packedSize = length;
	retval = 0x0000;  /* uncompressed */
	break;
    case 0x0004:  /* UNIX 12-bit compress */
#ifdef NO_UCOMP
	if (verbose) {
	    printf("[can't do 12-bit UNIX compress; storing]...");
	    fflush(stdout);
	} else {
	    printf(
	    "WARNING: can't do 12-bit compress; files stored uncompressed\n");
	}
	FCopy(srcfd, dstfd, length, buffer, TRUE);
	packedSize = length;
	retval = 0x0000;  /* uncompressed */
#else
	maxbits = 12;	/* global compress parameter */
	if (verbose) { printf("compressing...");  fflush(stdout); }
	/* packedSize set by compress() */
	if (u_compress(srcfd, dstfd, length) == OK)
	    retval = 0x0004;
	else
	    retval = 0x0004;	/* FIX this */
#endif
	break;

    case 0x0005:  /* UNIX 16-bit compress */
#ifdef NO_UCOMP
	if (verbose) {
	    printf("[can't do 16-bit UNIX compress; storing]...");
	    fflush(stdout);
	} else {
	    printf(
	    "WARNING: can't do 16-bit compress; files stored uncompressed\n");
	}
	FCopy(srcfd, dstfd, length, buffer, TRUE);
	packedSize = length;
	retval = 0x0000;  /* uncompressed */
#else
	maxbits = 16;	/* global compress parameter */
	if (verbose) { printf("compressing...");  fflush(stdout); }
	/* packedSize set by compress() */
	srcposn = (long) lseek(srcfd, (off_t) 0, S_REL);	/* save posn */
	dstposn = (long) lseek(dstfd, (off_t) 0, S_REL);
	if (u_compress(srcfd, dstfd, length) == OK) {
	    /* compress succeeded */
	    retval = 0x0005;
	} else {
	    /* compression failed */
	    if (verbose) { printf("storing..."); fflush(stdout); }
	    lseek(srcfd, (off_t) srcposn, S_ABS);	/* reposn files */
	    lseek(dstfd, (off_t) dstposn, S_ABS);
	    FCopy(srcfd, dstfd, length, buffer, TRUE);
	    packedSize = length;
	    retval = 0x0000;
	}
#endif
	break;

    default:
	fprintf(stderr, "\nUnknown compression method %d\n", thread_format);
	fprintf(stderr, "Aborting.\n");
	Quit(-1);
    }

    return (retval);
}


/*
 * Extract a range of bytes from one file into another, unpacking them.
 *
 * (hacked to unpack disks, also.  Forces the thread_eof to be the total
 *  size of the disk, since ShrinkIt doesn't really define it, esp for DOS 3.3
 *  disks).
 *
 * Set up stuff, then call the appropriate unpack routine.  Leaves the srcfd
 * positioned past the data to be unpacked; the calling routine should not
 * have to do any seeks.
 *
 * Returns TRUE if able to unpack, FALSE if not able to.  Note that srcfd
 * WILL be seeked even if the compression method is not handled.
 *
 * New uncompression routines should have the following characteristics:
 * - they should be able to uncompress a range of bytes from one file
 *   to another given two seeked file descriptors and a length parameter.
 * - they should return TRUE if they succeed and FALSE otherwise.  Special
 *   condition codes can be handled in the switch statement below.
 */
int
UnpackFile(srcfd, dstfd, THptr, thread_format, buffer)
int srcfd;	/* source file descriptor (must be open & lseek()ed) */
int dstfd;	/* destination file descriptor (must be open & lseek()ed) */
THblock *THptr;	/* pointer to thread structure */
int thread_format; /* how to unpack the bytes (NOT THptr->thread_format) */
onebyt *buffer;
{
    long length;
    fourbyt thread_eof,		/* #of bytes to output */
	    comp_thread_eof;	/* #of bytes in source */
    twobyt thread_crc;
    BOOLEAN retval = TRUE;  /* default to success */
    static char *procName = "UnpackFile";

    thread_eof = THptr->thread_eof;
    comp_thread_eof = THptr->comp_thread_eof;
    thread_crc = THptr->thread_crc;
    length = (long) comp_thread_eof;	/* type checking goes easier */

    switch (thread_format) {
    case 0x0000:  /* uncompressed */
	if (verbose) { printf("extracting...", thread_format); fflush(stdout);}
	FCopy(srcfd, dstfd, length, buffer, TRUE);
	break;

    case 0x0001:  /* unSQUeeze */
#ifdef NO_BLU
	if (verbose) {
	    printf("[can't unsqueeze - aborting]...");
	    fflush(stdout);
	} else {
	    printf("ERROR: can't unsqueeze; 'squ' files not extracted\n");
	}
	lseek(srcfd, (off_t)length, S_REL);  /* set file posn */
	retval = FALSE;
#else
	if (verbose) { printf("unsqueezing..."); fflush(stdout); }
	unpak_SQU(srcfd, dstfd, length);  /* thread_eof not needed */
#endif
	break;

    case 0x0002:  /* LZW (ShrinkIt) */
	if (verbose) { printf("unshrinking (I)..."); fflush(stdout); }
	unpak_SHK(srcfd, dstfd, comp_thread_eof, thread_eof, buffer, FALSE,
		thread_crc);
	break;

    case 0x0003:  /* LZW II (ShrinkIt) */
#ifdef TRY_II
	if (verbose) { printf("unshrinking (II)..."); fflush(stdout); }
	unpak_SHK(srcfd, dstfd, comp_thread_eof, thread_eof, buffer, TRUE,
		thread_crc);
#else
	if (verbose) {
	    printf("[can't unshrink type II - aborting]...");
	    fflush(stdout);
	} else {
	    printf(
		"ERROR: can't unshrink type II; 'sh2' files not extracted\n");
	}
	lseek(srcfd, (off_t)length, S_REL);  /* set file posn */
	retval = FALSE;
#endif
	break;

    case 0x0004:  /* 12-bit UNIX compress */
#ifdef NO_UCOMP
	if (verbose) {
	    printf("[can't undo 12-bit UNIX compress - aborting]...");
	    fflush(stdout);
	} else {
	    printf(
	"ERROR: can't undo 12-bit UNIX compress; 'u12' files not extracted\n");
	}
	lseek(srcfd, (off_t)length, S_REL);  /* set file posn */
	retval = FALSE;
#else
	if (verbose) { printf("uncompressing..."); fflush(stdout); }
	if (u_decompress(srcfd, dstfd, (long) comp_thread_eof) != OK)
	    retval = FALSE;
#endif
	break;

    case 0x0005:  /* 16-bit UNIX compress */
#ifdef NO_UCOMP
	if (verbose) {
	    printf("[can't undo 16-bit UNIX compress - aborting]...");
	    fflush(stdout);
	} else {
	    printf(
	"ERROR: can't undo 16-bit UNIX compress; 'u16' files not extracted\n");
	}
	lseek(srcfd, (off_t)length, S_REL);  /* set file posn */
	retval = FALSE;
#else
	if (verbose) { printf("uncompressing..."); fflush(stdout); }
	if (u_decompress(srcfd, dstfd, (long) comp_thread_eof) != OK)
	    retval = FALSE;
#endif
	break;

    default:
	fprintf(stderr, "Unknown uncompression method %d\n", thread_format);
	lseek(srcfd, (off_t)length, S_REL);  /* set file posn */
	retval = FALSE;
	break;
    }

    return (retval);
}

