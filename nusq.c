/*
 * nusq.c - Huffman squeeze/unsqueeze routines
 *
 * NuLib v3.2  March 1992  Freeware (distribute, don't sell)
 * By Andy McFadden (fadden@uts.amdahl.com)
 */
#ifdef APW
segment "Compress"
#endif

#include "nudefs.h"
#include "stdio.h"
#include <fcntl.h>

#ifdef UNIX
# include <sys/types.h>
#endif

#ifdef MSDOS     /* For file IO */
# include <io.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <errno.h>
#endif

#include "nuetc.h"

/*
 * usq.c - undo Huffman coding
 * Adapated from code By Marcel J.E. Mol
 * Based on sq3/usq2 by Don Elton
 *
 * Squeezed file format:
 *     2 bytes MAGIC
 *     2 bytes dummy ???  (maybe CRC or checksum; not checked)
 *     filename ended by \0
 *
 *     2 bytes node count
 *     node count node values, each 2 bytes
 *     squeezed data per byte
 *
 * NuFX SQueezed format includes only the node count, node values, and
 *   the data.  The BLU routines are expected to strip off the MAGIC,
 *   checksum, and filename before calling this.
 */

/*char *copyright = "@(#) usq.c  2.1 18/06/88  (c) M.J.E. Mol";*/
#define BUFSIZE 128
#define MAGIC	0xff76		       /* Squeezed file magic */
#define DLE 0x90		       /* repeat byte flag */
#define NOHIST 0		       /* no relevant history */
#define INREP 1 		       /* sending a repeated value */
#define SPEOF 256		       /* special endfile token */
#define NUMVALS 257		       /* 256 data values plus SPEOF */

/* global variable declarations */
char *sfn;			       /* squeezed file name */
struct nd {			       /* decoding tree */
    int child[2];		       /* left, right */
} node[NUMVALS];		       /* use large buffer */
int state;			       /* repeat unpacking state */
int bpos;			       /* last bit position read */
int curin;			       /* last byte value read */
int numnodes;			       /* number of nodes in decode tree */

static unsigned char fromc;	/* for use in text translation */
static BOOLEAN trbool;		/* BOOLEAN version of transfrom */


/* Get an integer from the input stream */
static twobyt
get_int(f)
FILE *f;
{
    twobyt val;

    val = (twobyt)getc(f);
    val += (twobyt)getc(f) << 8;
    return (val);
}


static int
getc_usq(f) 		       /* get byte from squeezed file */
FILE *f;			       /* file containing squeezed data */
{
    register short i;		       /* tree index */

    /* follow bit stream in tree to a leaf */
    for (i=0; (i <= 0x7fff) && (i>=0); )/* work down(up?) from root */
    {
	 if (++bpos > 7) {
	      if ((curin=getc(f)) == EOF)
		   return(EOF);
	      bpos = 0;

	      /* move a level deeper in tree */
	      i = node[i].child[1 & curin];
	 }
	 else i = node[i].child[1 & (curin >>= 1)];
    }

    /* decode fake node index to original data value */
    i = -(i + 1);

    /* decode special endfile token to normal EOF */
    return ((i==SPEOF) ? EOF : i);
}


/*  putc-ncr -- decode non-repeat compression.	Bytes are passed one
 *		at a time in coded format, and are written out uncoded.
 *		The data is stored normally, except that runs of more
 *		than two characters are represented as:
 *
 *			 <char> <DLE> <count>
 *
 *		With a special case that a count of zero indicates a DLE
 *		as data, not as a repeat marker.
 */
static void
putc_ncr(c, t)	       /* put NCR coded bytes */
unsigned char c;		       /* next byte of stream */
FILE *t;			       /* file to receive data */
{
    static int lastc;	      /* last character seen */

    /* if converting line terminators, do so now */
    if (trbool && (c == fromc))
#ifdef UNIX
	c = 0x0a;
#else
# ifdef APW
	c = 0x0d;
# else
	c = 0x0d;  /* No CRLF stuff in unSQueeze... sorry */
# endif
#endif

    switch (state) {		       /* action depends on our state */
	case NOHIST:		       /* no previous history */
	    if (c==DLE) 	       /* if starting a series */
		 state = INREP;        /* then remember it next time */
	    else putc(lastc=c, t);     /* else nothing unusual */
	    return;

	case INREP:		       /* in a repeat */
	    if (c)		       /* if count is nonzero */
		while (--c)	       /* then repeatedly ... */
		    putc(lastc, t);    /* ... output the byte */
	    else putc(DLE, t);	       /* else output DLE as data */
	    state = NOHIST;	       /* back to no history */
	    return;

	default:
	    fprintf(stderr, "%s: bad NCR unpacking state (%d)",
				prgName, state);
    }
}


static int
init_usq(f) 		       /* initialize Huffman unsqueezing */
FILE *f;			       /* file containing squeezed data */
{
    register int i;		       /* node index */

    switch (transfrom) {
    case -1:  /* no translation */
	trbool = 0;
	break;
    case 0:  /* from ProDOS */
	trbool = 1;
	fromc = 0x0d;
	break;
    case 1:  /* from UNIX */
	trbool = 1;
	fromc = 0x0a;
	break;
    case 2:  /* from MS-DOS... this needs fixing */
	trbool = 1;
	fromc = 0x0a;  /* just turn LFs into whatever... */
	break;
    default:  /* unknown */
	fprintf(stderr, "%s: unknown translation type %d\n", prgName, trbool);
	fprintf(stderr, "%s: assuming conversion from CR\n", prgName);
	trbool = 1;  /* should just ignore flag, but other procs do this */
	fromc = 0x0d;
	break;
    }

    bpos = 99;			       /* force initial read */
    numnodes = get_int(f);	       /* get number of nodes */

    if (numnodes<0 || numnodes>=NUMVALS) {
	 fprintf(stderr, "%s: usq: archived file has invalid decode tree\n",
							prgName);
	 return (-1);
    }

    /* initialize for possible empty tree (SPEOF only) */
    node[0].child[0] = -(SPEOF + 1);
    node[0].child[1] = -(SPEOF + 1);

    for (i=0; i<numnodes; ++i) {	/* get decoding tree from file */
	 node[i].child[0] = get_int(f);
	 node[i].child[1] = get_int(f);
    }

    return (0);
}


/*
 * Unsqueeze a file
 */
static int
unsqueeze(sfp, dfp)
FILE *sfp, *dfp;
{
    register int i;
    register int c;			/* one char of stream */

    state = NOHIST;		       /* initial repeat unpacking state */

    if (init_usq(sfp))		       /* init unsqueeze algorithm */
	return 1;
    while ((c=getc_usq(sfp)) != EOF)   /* and unsqueeze file */
	 putc_ncr(c, dfp);

    return (0);			       /* file is okay */
}


/*
 * main entrance to unsqueeze
 *
 * We reset the file posn to where it should be according to "length"; note
 * that "length" is not actually used by the unsqueeze routines.  We have
 * do to this because fdopen() sticks about 8K or so in a buffer...
 *
 * Note also that we dup() the file descriptors before starting.  This
 * is so that we can cleanly fclose() the file descriptors when we're done,
 * but still keep the file open.
 */
void
unpak_SQU(srcfd, dstfd, length)
int srcfd, dstfd;
long length;  /* #of bytes we're expected to read */
{
    FILE *srcfp, *dstfp;		/* File pointers for squ/dest file */
    int srcfd2, dstfd2;
    long finalposn;
    static char *procName = "unpak_SQU";

    finalposn = lseek(srcfd, (off_t)0, S_REL) + length;  /* where we should end up */
    srcfd2 = dup(srcfd);
    dstfd2 = dup(dstfd);
    if ((srcfp = fdopen(srcfd2, FREAD_STR)) == NULL)
	Fatal("Can't fdopen() archive", procName);
    if ((dstfp = fdopen(dstfd2, FWRITE_STR)) == NULL)
	Fatal("Can't fdopen() dest file", procName);

    unsqueeze(srcfp, dstfp);  /* unsqueeze the file */
    fclose(srcfp);  /* (was fflush) (this isn't really necessary) */
    fclose(dstfp);  /* (was fflush) This is *very* necessary */

    if (lseek(srcfd, (off_t)finalposn, S_ABS) < 0)  /* set file posn back */
	Fatal("Can't reset final posn", procName);
    /* note that this lets things continue even if unSQueezing failed */
}

