/*
 * nushk.c - P8 ShrinkIt compress/uncompress routines
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

#include "nuread.h"  /* need CalcCRC() */
#include "nupak.h"
#include "nuetc.h"

#ifdef UNIX
# ifdef SYSV
#  define BCOPY(src, dst, count) memcpy(dst, src, count)
# else
#  define BCOPY(src, dst, count) bcopy(src, dst, count);
# endif
#else
# ifdef APW
#  define BCOPY(src, dst, count) memcpy(dst, src, count)
# endif
# ifdef MSDOS	/* you may need to change this to memcpy() */
#  define BCOPY(src, dst, count) bcopy(src, dst, count);
/*#  define BCOPY(src, dst, count) memcpy(dst, src, count)*/
# endif
/* +PORT+ */
#endif

#define BLKSIZ	4096
/*#define DEBUG 	/* do verbose debugging output */
/*#define DEBUG1	/* debugging output in main routine */

static onebyt *ibuf;	/* large buffer (usually set to packBuffer) */
onebyt lbuf[BLKSIZ+7];	/* temporary buffer for storing data after LZW */
onebyt rbuf[BLKSIZ+7];	/* temporary buffer for storing data after RLE */


/*
 * P8 ShrinkIt compression routines
 * Copyright 1989 Kent Dickey
 *
 * C translation by Kent Dickey / Andy McFadden
 */

#define ESCAPE_CHAR	0xdb
#define HSIZE 5119		/* Must be prime */

typedef struct ht {
	int entry;
	int prefix;
	onebyt chr;
};
struct ht *htab = NULL;		/* allocated first time through */


int s_at_bit;
int s_at_byte;

int mask[] = {	0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f,
		0xff, 0x1ff, 0x3ff, 0x7ff, 0xfff };

int bit_tab[] = { 0,9,10, 10, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12 };


/* hash function from UNIX compress */
int hashf[256] = {
	0x0000, 0x0204, 0x0408, 0x060c, 0x0810, 0x0a14, 0x0c18, 0x0e1c,
	0x0020, 0x0224, 0x0428, 0x062c, 0x0830, 0x0a34, 0x0c38, 0x0e3c,
	0x0040, 0x0244, 0x0448, 0x064c, 0x0850, 0x0a54, 0x0c58, 0x0e5c,
	0x0060, 0x0264, 0x0468, 0x066c, 0x0870, 0x0a74, 0x0c78, 0x0e7c,
	0x0080, 0x0284, 0x0488, 0x068c, 0x0890, 0x0a94, 0x0c98, 0x0e9c,
	0x00a0, 0x02a4, 0x04a8, 0x06ac, 0x08b0, 0x0ab4, 0x0cb8, 0x0ebc,
	0x00c0, 0x02c4, 0x04c8, 0x06cc, 0x08d0, 0x0ad4, 0x0cd8, 0x0edc,
	0x00e0, 0x02e4, 0x04e8, 0x06ec, 0x08f0, 0x0af4, 0x0cf8, 0x0efc,
	0x0100, 0x0304, 0x0508, 0x070c, 0x0910, 0x0b14, 0x0d18, 0x0f1c,
	0x0120, 0x0324, 0x0528, 0x072c, 0x0930, 0x0b34, 0x0d38, 0x0f3c,
	0x0140, 0x0344, 0x0548, 0x074c, 0x0950, 0x0b54, 0x0d58, 0x0f5c,
	0x0160, 0x0364, 0x0568, 0x076c, 0x0970, 0x0b74, 0x0d78, 0x0f7c,
	0x0180, 0x0384, 0x0588, 0x078c, 0x0990, 0x0b94, 0x0d98, 0x0f9c,
	0x01a0, 0x03a4, 0x05a8, 0x07ac, 0x09b0, 0x0bb4, 0x0db8, 0x0fbc,
	0x01c0, 0x03c4, 0x05c8, 0x07cc, 0x09d0, 0x0bd4, 0x0dd8, 0x0fdc,
	0x01e0, 0x03e4, 0x05e8, 0x07ec, 0x09f0, 0x0bf4, 0x0df8, 0x0ffc,
	0x0200, 0x0004, 0x0608, 0x040c, 0x0a10, 0x0814, 0x0e18, 0x0c1c,
	0x0220, 0x0024, 0x0628, 0x042c, 0x0a30, 0x0834, 0x0e38, 0x0c3c,
	0x0240, 0x0044, 0x0648, 0x044c, 0x0a50, 0x0854, 0x0e58, 0x0c5c,
	0x0260, 0x0064, 0x0668, 0x046c, 0x0a70, 0x0874, 0x0e78, 0x0c7c,
	0x0280, 0x0084, 0x0688, 0x048c, 0x0a90, 0x0894, 0x0e98, 0x0c9c,
	0x02a0, 0x00a4, 0x06a8, 0x04ac, 0x0ab0, 0x08b4, 0x0eb8, 0x0cbc,
	0x02c0, 0x00c4, 0x06c8, 0x04cc, 0x0ad0, 0x08d4, 0x0ed8, 0x0cdc,
	0x02e0, 0x00e4, 0x06e8, 0x04ec, 0x0af0, 0x08f4, 0x0ef8, 0x0cfc,
	0x0300, 0x0104, 0x0708, 0x050c, 0x0b10, 0x0914, 0x0f18, 0x0d1c,
	0x0320, 0x0124, 0x0728, 0x052c, 0x0b30, 0x0934, 0x0f38, 0x0d3c,
	0x0340, 0x0144, 0x0748, 0x054c, 0x0b50, 0x0954, 0x0f58, 0x0d5c,
	0x0360, 0x0164, 0x0768, 0x056c, 0x0b70, 0x0974, 0x0f78, 0x0d7c,
	0x0380, 0x0184, 0x0788, 0x058c, 0x0b90, 0x0994, 0x0f98, 0x0d9c,
	0x03a0, 0x01a4, 0x07a8, 0x05ac, 0x0bb0, 0x09b4, 0x0fb8, 0x0dbc,
	0x03c0, 0x01c4, 0x07c8, 0x05cc, 0x0bd0, 0x09d4, 0x0fd8, 0x0ddc,
	0x03e0, 0x01e4, 0x07e8, 0x05ec, 0x0bf0, 0x09f4, 0x0ff8, 0x0dfc,
};


/*
 * Output code to buffer.
 */
int
put_code(code, ent, bfr)
int code, ent;
onebyt *bfr;
{
	int lo_byte;
	register long mycode;
	int bits;
	register onebyt *cp;	/* opt: points to bfr[s_at_byte] */

	bits = bit_tab[(ent >> 8)];
	if (((s_at_bit + bits + 7) >> 3) + s_at_byte > BLKSIZ) {
		return(-1);
	}
	mycode = (long)(code & mask[bits]);
#ifdef DEBUG2
	fprintf(stderr,"Byte: %d, %lx\n", s_at_byte, mycode);
#endif
/*	lo_byte = bfr[s_at_byte] & mask[s_at_bit];*/
	cp = bfr + s_at_byte;		/* - */
	lo_byte = *cp & mask[s_at_bit]; /* - */
	if (s_at_bit != 0) {
		mycode <<= s_at_bit;
	}
/*	bfr[s_at_byte++] = (onebyt) lo_byte | (onebyt) (mycode & 0xff);*/
	*cp = (onebyt) lo_byte | (onebyt) (mycode & 0xff); /* - */
	s_at_byte++, cp++;
/*	bfr[s_at_byte] = (onebyt) ((mycode >>= 8) & 0xff);*/
	*cp = (onebyt) ((mycode >>= 8) & 0xff);	/* - */
	if ((s_at_bit += bits) >= 16) {
/*		bfr[++s_at_byte] = (char)((mycode >>= 8) & 0xff);*/
		cp++, s_at_byte++;  /* - */
		*cp = (onebyt) ((mycode >>= 8) & 0xff);  /* - */
	}
	s_at_bit &= 0x07;

	return(0);
}


/*
 * Try LZW compression on the buffer.
 *
 * Compresses from "buffer" to "outbuf".  "inlen" is the #of bytes of data in
 * "buffer."  Returns the #of bytes of data placed into "outbuf", or -1 on
 * failure.
 */
int
do_LZW(bufr, inlen, outbuf)
onebyt *bufr;
int inlen;
onebyt *outbuf;
{
	register int index;
	register onebyt k;
	register onebyt *inbuf, *endbuf;
	register struct ht *htp;	/* opt: htab[index] */
	int ent, prefix, hashdelta;

	s_at_byte = 0;
	s_at_bit =0;
	ent = 0x101;
	inbuf = bufr;
	endbuf = bufr + inlen;


	k = ((char)*inbuf++);
Loop0:
	prefix = (int)k;
Loop1:
	if (inbuf >= endbuf) {
		if (put_code(prefix, ent, outbuf) < 0) {
			return(BLKSIZ+2);
		}
		if (s_at_bit == 0) return(s_at_byte);
		else		   return(s_at_byte+1);
	}
	k = (onebyt)*inbuf++;
#ifdef OLD_HASH
	index = (prefix + (k<<4)) & 0xfff;
#else
	index = prefix ^ hashf[k];		/* note index always < 4096 */
#endif

Check_ent:
	htp = htab + index;
	if (htp->entry == 0) {
		/* Entry is 0... */
		if (put_code(prefix, ent, outbuf) < 0) {
			return(-1);	/* failed */
		}
		htp->entry = ent++;
		htp->prefix = prefix;
		htp->chr = k;
		goto Loop0;
	}
	else if (htp->prefix == prefix) {  /* - */
		/* Same prefix... */
		if (htp->chr == k) {
			/* It's HERE!  Yeah! */
			prefix = htp->entry;
			goto Loop1;
		}
		goto Sec_hash;
	}
		/* Check for more...secondary hash on index */
	  else {
Sec_hash:
#ifdef OLD_HASH
		/*index = (index + (unsigned int)(k) + 1) % HSIZE;*/
		index += (unsigned int)(k) +1;
		if (index >= HSIZE) index -= HSIZE;
#else
		hashdelta = (0x120 - k) << 2;
		if (index >= hashdelta)
			index -= hashdelta;
		else
			index += (HSIZE - hashdelta);
#endif
#ifdef DEBUG2
		fprintf(stderr,"New ind: %d, k=%d\n", index, (unsigned int)k);
#endif
		goto Check_ent;
	}
}


/*
 * Clear out the hash table.
 */
void
ClearTab()
{
	register int i;
	register struct ht *htp;	/* opt: htab[i] */

/*	for(i=0; i < HSIZE; i++)*/
/*		htab[i].entry = 0;*/
	htp = htab;			/* - */
	for (i = HSIZE; i; htp++, i--)	/* - */
		htp->entry = 0;		/* - */
}


/*
 * Do run-length encoding
 *
 * Takes input from srcptr, and writes to dstptr.  Maximum expansion is
 * (BLKSIZ / 2) + (BLKSIZ / 2) * 3 == 2 * BLKSIZ
 * Output of form  <DLE> char count  where count is #of bytes -1.
 *
 * This really isn't very pretty, but it works.
 */
int
do_RLE(srcptr, dstptr)
register onebyt *srcptr, *dstptr;
{
#define ALT_RLE		/* testing */
#ifndef ALT_RLE
    register int found, scount /*, dcount*/;
    register onebyt c, lastc, tlastc;
    onebyt *dststart = dstptr;

    c = *(srcptr++);  scount = 1;
    /*dcount = 0;*/
    found = 1;  /* one char has been found */
    lastc = '\0';
    while (scount < BLKSIZ) {
	tlastc = lastc;
	lastc = c;
	c = *(srcptr++);  scount++;

	if (found == 1) {  /* no run found */
	    if (c != lastc) {  /* no run starting */
		if (lastc == ESCAPE_CHAR) {
		    *(dstptr++) = ESCAPE_CHAR;  /*dcount++;*/
		    *(dstptr++) = lastc;  /*dcount++;*/
		    *(dstptr++) = 0;  /*dcount++;*/  /* found one */
		} else {
		    *(dstptr++) = lastc;  /*dcount++;*/
		}
		found = 1;
	    } else {
		found = 2;  /* they matched, so two in a row */
	    }

	} else if (found == 2) {  /* got two, try for three */
	    if (c != lastc) {  /* only got two in a row */
		if (lastc == ESCAPE_CHAR) {  /* and tlastc as well */
		    *(dstptr++) = ESCAPE_CHAR;  /*dcount++;*/
		    *(dstptr++) = lastc;  /*dcount++;*/
		    *(dstptr++) = 1;  /*dcount++;*/  /* found two */
		} else {
		    *(dstptr++) = tlastc;  /*dcount++;*/
		    *(dstptr++) = lastc;  /*dcount++;*/
		}
		found = 1;
	    } else {  /* found 3, got a run going */
		found = 3;
	    }

	} else {  /* (found >= 3), got a run going */
	    if (c == lastc) {  /* found another */
		found++;
	    }
	    if ((c != lastc) || (found > 256)) {  /* end, or too many */
		*(dstptr++) = ESCAPE_CHAR;  /*dcount++;*/
		*(dstptr++) = lastc;  /*dcount++;*/
		*(dstptr++) = (found > 256) ? 255 : found-1;
		/*dcount++;*/
		found = 1;  /* c has something other than the run char	*/
			    /* or found is 257-256 = 1		 	*/
	    }
	}
    }  /* while */

    /* reached end of buffer; flush what was left */
    if (found == 1) {
	if (c == ESCAPE_CHAR) {
	    *(dstptr++) = ESCAPE_CHAR;  /*dcount++;*/
	    *(dstptr++) = c;  /*dcount++;*/
	    *(dstptr++) = 0;  /*dcount++;*/
	} else {
	    *(dstptr++) = c;  /*dcount++;*/
	}

    } else if (found == 2) {
	/* maybe have if lastc == c == ESCAPE_CHAR? */
	if (lastc == ESCAPE_CHAR) {
	    *(dstptr++) = ESCAPE_CHAR;  /*dcount++;*/
	    *(dstptr++) = lastc;  /*dcount++;*/
	    *(dstptr++) = 0;  /*dcount++;*/
	} else {
	    *(dstptr++) = lastc;  /*dcount++;*/
	}
	if (c == ESCAPE_CHAR) {
	    *(dstptr++) = ESCAPE_CHAR;  /*dcount++;*/
	    *(dstptr++) = c;  /*dcount++;*/
	    *(dstptr++) = 0;  /*dcount++;*/
	} else {
	    *(dstptr++) = c;  /*dcount++;*/
	}

    } else {  /* found >= 3, in the middle of processing a run */
	*(dstptr++) = ESCAPE_CHAR;  /*dcount++;*/
	*(dstptr++) = c;  /*dcount++;*/
	*(dstptr++) = found-1;  /*dcount++;*/
    }

/*    return (dcount);*/
    return (dstptr - dststart);
#else /*ALT_RLE*/
    /*
     * This was an attempt to write a faster do_RLE routine.  However,
     * the profiler on my machine showed it to be slower than the big
     * wad of stuff above.  I decided to leave the code here in case somebody
     * wants to play with it.
     */
    register onebyt *scanptr;
    onebyt *endptr, *dststart;
    register onebyt c;
    register int i, count;

    endptr = srcptr + BLKSIZ;	/* where the source ends */
    dststart = dstptr;		/* where the destination begins */

    while (srcptr < endptr) {
	c = *srcptr;
	scanptr = srcptr+1;
	count = 1;

	while (*scanptr == c && scanptr < endptr)
	    scanptr++, count++;
	if (count > 3) {
	    i = count;
	    do {
		*(dstptr++) = ESCAPE_CHAR;
		*(dstptr++) = c;
		if (i > 256) {			/* was count */
		    *(dstptr++) = 255;
		    i -= 256;
		} else {
		    *(dstptr++) = i-1;		/* was count-1 */
		    break;
		}
	    } while (i > 0);
	} else {
	    if (c == ESCAPE_CHAR) {	/* special case: 1-3 0xDBs */
		*(dstptr++) = ESCAPE_CHAR;
		*(dstptr++) = ESCAPE_CHAR;
		*(dstptr++) = count-1;	/* count == 0 is legal */
	    } else {
		i = count;
		while (i--)
		    *(dstptr++) = c;
	    }
	}

	srcptr += count;
    }

#ifdef DEBUG
    if (srcptr > endptr)
	printf("BUG: srcptr > endptr in do_RLE\n");
#endif

    return (dstptr - dststart);

#endif /*ALT_RLE*/
}


/*
 * Main compression entry point.
 *
 * Returns actual thread_format used.
 *
 * Note that "copybuf" should be at least twice as big as BLKSIZ.
 */
long
pak_SHK(srcfd, dstfd, length, copybuf)
int srcfd, dstfd;
long length;		/* uncompressed size */
onebyt *copybuf;
{
    unsigned int partial;   /* size for partial read/write */
    onebyt *rptr, *out_buf;
    register int idx;
    onebyt scratch[8];
    long srcposn,	/* start in source file */
	 startposn,	/* start in dest file */
	 endposn;	/* end in dest file */
    long unc_len = length,
	 comp_len = 0L;
    twobyt CRC;
    int rlesize, lzwsize, out_size;	/* length after compression */
    int sc;  /* spin counter */
    static char *procName = "pak_SHK";

    if (htab == NULL)
	htab = (struct ht *) Malloc(HSIZE * sizeof(struct ht));

    CRC = 0;
    if ((srcposn = lseek(srcfd, (off_t) 0, S_REL)) < 0)	/* only used if */
	Fatal("Bad seek (srcposn)", procName);		/* compress fails */
    if ((startposn = lseek(dstfd, (off_t) 0, S_REL)) < 0)
	Fatal("Bad seek (startposn)", procName);
    lseek(dstfd, (off_t) 4, S_REL);  /* leave room for 4-byte header */
    comp_len += 4L;

    sc = 0;
    do {  /* have to handle when length == 0L */
	if (length > (long) BLKSIZ) {
	    partial = (unsigned int) BLKSIZ;
	    length -= (long) BLKSIZ;
	} else {
	    partial = (unsigned int) length;
	    length = 0L;
	    for (idx = partial; idx < BLKSIZ; idx++)  /* fill in zeroes */
		*(copybuf + idx) = 0;
	}

	if (partial > 0) {  /* should work anyway, but let's be careful */
	    if (read(srcfd, copybuf, partial) != partial)
		Fatal("Source read failed", procName);
	}
	/* calc CRC on all 4096 bytes */
	CRC = CalcCRC(CRC, (onebyt *) copybuf, BLKSIZ); 
	rlesize = do_RLE(copybuf, copybuf + BLKSIZ+1);  /* pack 4096 bytes */
	if (rlesize < 0x1000) {  /* did it pack or expand? */
	    rptr = copybuf + BLKSIZ+1;  /* use packed version */
	} else {
	    rlesize = 0x1000;  /* just store original */
	    rptr = copybuf;
	}
	ClearTab();
	lzwsize = do_LZW(rptr, rlesize, lbuf);	/* compress from rle to lzw */
	if ((lzwsize > rlesize) || (lzwsize < 0)) {
	    /* lzw failed, use rle'd copy */
	    scratch[2] = 0;
	    out_size = rlesize;
	    out_buf = rptr;
	} else {
	    /* lzw succeeded, use it */
	    scratch[2] = 1;		/* LZW on */
	    out_size = lzwsize;
	    out_buf = lbuf;
	}
	scratch[0] = (onebyt) (rlesize & 0x00ff);	/* NOT out_size */
	scratch[1] = (onebyt) ((rlesize >> 8) & 0x00ff);
	if (write(dstfd, scratch, 3) != 3)
	    Fatal("Dest hdr write failed", procName);
	comp_len += 3;
	comp_len += out_size;
	if (comp_len > unc_len)
	    goto bad_compress;		/* you didn't see this */
	if (write(dstfd, out_buf, out_size) != out_size)  /* need to do CRLF */
	    Fatal("Dest write failed", procName);

	sc++;
	if (sc == 15) {
	    sc = 0;
	    Spin();
	}
    } while (length != 0L);

    if ((endposn = lseek(dstfd, (off_t) 0, S_REL)) < 0)
	Fatal("Bad seek (now)", procName);
    if (lseek(dstfd, (off_t) startposn, S_ABS) < 0)
	Fatal("Bad seek (to4)", procName);
    scratch[0] = (char) CRC;
    scratch[1] = (char) (CRC >> 8);
    scratch[2] = 0;
    scratch[3] = ESCAPE_CHAR;
    if (write(dstfd, scratch, 4) != 4)
	Fatal("Dest hdr write failed", procName);
    if (lseek(dstfd, (off_t) endposn, S_ABS) < 0)
	Fatal("Bad seek (last)", procName);

    if (comp_len != endposn - startposn) {
	printf(
	    "internal error: comp_len=%ld, endposn=%ld, startposn=%ld (%ld)\n",
		comp_len, endposn, startposn, endposn - startposn);
    }
    packedSize = comp_len;
    return (0x0002);	/* SHK packing */

bad_compress:	/* I'm too lazy to do a procedure call... */

    if (verbose) { printf("storing...");  fflush(stdout); }
    if (lseek(srcfd, (off_t) srcposn, S_ABS) < 0)
	Fatal("Bad seek (srcposn in bad_compress)", procName);
    if (lseek(dstfd, (off_t) startposn, S_ABS) < 0)
	Fatal("Bad seek (startposn in bad_compress)", procName);
    FCopy(srcfd, dstfd, unc_len, copybuf, FALSE);
    packedSize = unc_len;
    return (0x0000);	/* no compression */
}


/*
 * P8 ShrinkIt uncompression routines
 *
 * Copyright 1989 Kent Dickey
 * C translation by Kent Dickey / Andy McFadden
 * Modifications for LZW-II designed by Andy Nicholas
 *
 * C decoder for LZW-II by Frank Petroski / Kent Dickey (simultaneously
 * and independently).  Speed optimizations by Kent Dickey.
 */

/*static int inf;  /* to make Getc() calls happy */
static BOOLEAN type2;	/* true if working with LZW-II format */

static onebyt escape_char;

typedef struct {
    unsigned char chr;
    int prefix;
} Table_ent;

static Table_ent Real_tab[BLKSIZ-256];  /* first 256 don't exist */
static Table_ent *Table;

static int Mask_tab[17] = {
	0x0000, 0x01ff, 0x03ff, 0x03ff, 0x07ff,
	0x07ff, 0x07ff, 0x07ff, 0x0fff, 0x0fff,
	0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
	0x0fff, 0x0fff
};
static int Number[17] = {
/*	0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,4 };*/
	8,9,10,10,11,11,11,11,12,12,12,12,12,12,12,12,12 };

static onebyt *Stack;	/* simulated stack <= 64 for LZW-I, <= 4096 for II */
static int out_bytes, stack_ptr, entry, at_bit, at_byte;
static onebyt last_byte;  /* used in get_code */
static int reset_fix;	/* fix problem unpacking certain LZW-II archives */


/* fake getc() */
# define Getc() *(ibuf++)


/*
 * Stack operations; used by undo_LZW and undo_LZW_2
 */
#ifdef DEBUG
# define push(a_byte) \
    { \
	if (stack_ptr - Stack > 4096) { \
	    printf("\n*** stack_ptr exceeded 4096 in push() [%d]\n", \
		(int) (stack_ptr - Stack));\
	    exit (-1); \
	} \
	*(stack_ptr++) = (onebyt) a_byte; \
    }
#else
# define push(a_byte) *(stack_ptr++) = (onebyt) a_byte;
#endif

#ifdef DEBUG
# define dump_stack(buffer) \
    { \
	printf("--- Going to dump stack, stack_ptr = %d, out_bytes = %d\n", \
	    (int) (stack_ptr - Stack), out_bytes); \
	while (stack_ptr-- > stack_start) { \
	    *(buffer++) = *stack_ptr; \
	} \
    }
#else
# define dump_stack(buffer) \
    while (stack_ptr-- > stack_start) { \
	*(buffer++) = *stack_ptr; \
    }
#endif


/*
 * Decipher LZW codes.
 */
int
get_code()
{
    register unsigned int num_bits, old_bit, last_bit;
    long value, mask;
/*    onebyt byte1, byte2, byte3;*/  /* get compressed chars... */
    long byte1, byte2, byte3;  /* - */

#ifdef DEBUG
    printf("ENT: bit=%d byte=%-4d last_byte=$%.2x ",
	at_bit, at_byte, last_byte);
    printf("Entry: %.4x \n", entry);
#endif

    num_bits = ((entry+1) >> 8);  /* get hi-byte of entry */
/*    last_bit = at_bit + Number[num_bits] + 8;*/
    last_bit = at_bit + Number[num_bits];
    old_bit = at_bit;
#ifdef DEBUG
    if (at_byte >= BLKSIZ) {
	fprintf(stderr, "at_byte exceeded BLKSIZ (4096) in get_code()\n");
	exit (-1);
    }
#endif
    if (at_bit == 0)
	last_byte = Getc();
/*    byte1 = last_byte;*/  /* first byte = last one used */
    byte1 = (long) last_byte;  /* - */
    last_byte = Getc();        /* - */
/*    byte2 = Getc(inf);*/
    byte2 = ((long) last_byte) << 8;  /* - */
    if (last_bit > 16) {  /* get 3rd byte if nec. */
/*	byte3 = Getc(inf);*/
/*	last_byte = byte3;*/
	last_byte = Getc();  /* - */
	byte3 = ((long) last_byte) << 16;  /* - */
    } else {
	byte3 = 0;
/*	last_byte = byte2;*/
    }
/*    value = ((((long)byte3 << 8) + (long)byte2) << 8) + (long)byte1;*/
    value = byte3 + byte2 + byte1;  /* - */

    mask = (long) Mask_tab[num_bits];
    at_byte += (last_bit >> 3);  /* new byte */
    at_bit = (last_bit & 0x07);

#ifdef DEBUG
    printf("| EX: value=$%.6x mask=$%.4x return=$%.3x\n",
	value, mask, ((value >> old_bit) & mask));
#endif
#ifdef ZERO_SHIFT_BAD
    if (old_bit)
	return ((value >> old_bit) & mask);
    else
	return (value & mask);  /* shifting by zero may be undefined */
#else
    return ((value >> old_bit) & mask);
#endif /*ZERO_SHIFT_BAD*/
}


/*
 * Un-LZW a range of bytes
 *
 * Reads data with get_code() and stores the output in "buffer".
 */
void
undo_LZW(buffer, length)
unsigned char *buffer;  /* where to put output */
int length;  /* uncompressed length of output */
{
    register int oldc, incode, finalc, ptr;
    register onebyt *endbuf, *stack_ptr, *stack_start;

    /* initialize variables */
    Table = Real_tab-256;
    entry = 0x101;  /* start at $101 */
    at_bit = at_byte = 0;
    endbuf = buffer + length;
    stack_start = stack_ptr = Stack;

    last_byte = 0;  /* init last_byte */
    oldc = incode = get_code(/*buffer*/);
    finalc = (oldc & 0xff);
    *(buffer++) = (onebyt) incode;

    /* main loop */
    while (buffer < endbuf) {
	incode = ptr = get_code(/*buffer*/);
	if (ptr >= entry) {			/* handle KwKwK case */
	    push(finalc);
	    ptr = oldc;
	}

	while (ptr > 0xff) {			/* fill the stack */
	    push(Table[ptr].chr);
	    ptr = Table[ptr].prefix;
	}

	/* ptr is now < 0x100 */
	finalc = ptr;
	*(buffer++) = (onebyt) finalc;
	while (stack_ptr > stack_start)		/* dump the stack */
		*(buffer++) = *(--stack_ptr);
	Table[entry].chr = (finalc & 0xff);  /* mask to get unsigned?? byte */
	Table[entry].prefix = oldc;
	entry++;
	oldc = incode;
    }
}


/*
 * Un-LZW-II a range of bytes
 *
 * Reads data with get_code() and stores the output in "buffer".  Has
 * additional code to support LZW-II's table clears.
 */
void
undo_LZW_2(buffer, length)
unsigned char *buffer;  /* where to put output */
int length;  /* uncompressed length of output */
{
    register int oldc, incode, finalc, ptr;
    register onebyt *endbuf, *stack_ptr, *stack_start;

    /* initialize variables */
    at_bit = at_byte = 0;
/*    out_bytes = 0;*/
/*    stack_ptr = 0;*/
    endbuf = buffer + length;
    stack_start = stack_ptr = Stack;

    last_byte = 0;  /* init last_byte */

    /* main loop */
/*    while (out_bytes < length) {*/
    while (buffer < endbuf) {  /* - */

	if (entry == 0x101 && !reset_fix) {	/* table is really empty */
	    oldc = incode = get_code(/*buffer*/);
	    finalc = (oldc & 0xff);
/*	    *(buffer + out_bytes++) = (unsigned char) incode;*/
	    *(buffer++) = (onebyt) incode;  /* - */
	    if (buffer >= endbuf) {	/* buffer is full?  If so, we */
		reset_fix = 1;		/* want to skip get_code() next time */
		break;			/* we come through for next chunk */
	    }
	}

	incode = ptr = get_code(/*buffer*/);
	if (incode == 0x100) {		/* Clear table code */
	    entry = 0x101;  /* start at $101 */
	    Table = Real_tab-256;
	    reset_fix = 0;
	    continue;
	}

	if (ptr >= entry) {
	    push(finalc);
	    ptr = oldc;
	}
	while (ptr > 0xff) {
	    push(Table[ptr].chr);
	    ptr = Table[ptr].prefix;
	}

	/* ptr is now < 0x100 */
	finalc = ptr;
/*	push(finalc);*/
/*	dump_stack(buffer);*/
	*(buffer++) = (onebyt) finalc;    /* - */
	while (stack_ptr > stack_start)   /* - */
	    *(buffer++) = *(--stack_ptr); /* - */
	Table[entry].chr = (finalc & 0xff);  /* mask to get unsigned?? byte */
	Table[entry].prefix = oldc;
	entry++;
	oldc = incode;
    }
}


/*
 * Second pass... undo the Run Length Encoding.
 *
 * Copy data from inbuffer to outbuffer.  Keep going until we've got
 * exactly BLKSIZ bytes.  Note that this uses codes of the form
 *   <DLE> char count
 * which is different from some other programs.
 */
void
undo_RLE(inbuffer, outbuffer)
unsigned char *inbuffer, *outbuffer;
/*int length;  /* how many bytes from LZW; just to make sure... */
{
    register onebyt c;
    register int count;  /* count is RLE reps */
    register onebyt *outbufend;

#ifdef DEBUG
    /*printf("Starting undo_RLE, length = %d\n", length);*/
#endif
    outbufend = outbuffer + BLKSIZ;
    while (outbuffer < outbufend) {
	c = *(inbuffer++);  /*length--;*/
	if (c == (onebyt) escape_char) {
	    c = *(inbuffer++);  /*length--;*/
	    count = *(inbuffer++);  /*length--;*/
	    while (count-- >= 0) {
		*(outbuffer++) = c;  /*Putc(c, outf);*/
	    }
	} else {
	    *(outbuffer++) = c;  /*Putc(c, outf);*/
	}
    }

    if (outbuffer != outbufend)
	fprintf(stderr, "internal error: bad undo_RLE\n");
#ifdef DEBUG
/*    printf("Exiting undo_RLE, length = %d (should be 0), total = %d (4096)\n",
	length, outbufend-outbuffer);*/
#endif
}


/*
 * Main entry point.
 *
 * This is among the more hellish things I've written.  Uses
 *   a large buffer for efficiency reasons, and unpacks a stream of bytes.
 *
 * If you find this hard to understand, imagine what it was like to debug.
 */
void
unpak_SHK(srcfd,dstfd,comp_thread_eof,thread_eof,buffer, use_type2, thread_crc)
int srcfd, dstfd;
fourbyt comp_thread_eof, thread_eof;
register onebyt *buffer;
BOOLEAN use_type2;		/* true if we should expect LZW-II */
twobyt thread_crc;
{
    twobyt CRC, blkCRC;
    onebyt vol;
    onebyt *wrbuf;  /* points to buffer we're about to write */
    short unlen, lzwflag, rleflag, complen;	/* should be short */
    unsigned int partial, toread, still_in_buf /*, crcsize*/;
    fourbyt tmp4;  /* temporary 4-byte variable */
    int cc;
    static char *procName = "unpak_SHK";

    if (Stack == NULL)
	Stack = (onebyt *) Malloc(4096);

    type2 = use_type2;

    if (type2)
	CRC = 0xffff;			/* different CRC for LZW-II */
    else
	CRC = 0;

    /* initialize variables for LZW-II */
    Table = Real_tab-256;
    entry = 0x101;  /* start at $101 */
    reset_fix = 0;

    /* read min(PAKBUFSIZ, comp_thread_eof) bytes into buffer */
    if (comp_thread_eof > (fourbyt) PAKBUFSIZ) {
	toread = (unsigned int) PAKBUFSIZ;
	comp_thread_eof -= (fourbyt) PAKBUFSIZ;
    } else {
	toread = (unsigned int) comp_thread_eof;  /* read it all... */
	comp_thread_eof = (fourbyt) 0;
    }

    /* do initial read */
#ifdef DEBUG1
    printf("initial read = %u\n", toread);
#endif
    if ((cc = read(srcfd, buffer, toread)) < toread) {
#ifdef DEBUG1
	printf("Only read %d bytes\n", cc);
#endif
	Fatal("Bad read during uncompress", procName);
    }
    ibuf = buffer;  /* set input pointer to start of buffer */

    /* get header data */
    if (type2) {
	blkCRC = thread_crc;
    } else {
	blkCRC = Getc();
	blkCRC += (Getc() << 8);
    }
    vol = (char) Getc();  /* disk volume #; not used here */
    escape_char = (char) Getc();  /* RLE delimiter */

#ifdef DEBUG1
    printf("vol = %d, escape_char = %x\n", vol, escape_char);
#endif

    /*
     * main loop
     */
    while (thread_eof != (fourbyt) 0) {

	/* note "unlen" is un-LZWed length (i.e., after RLE) */
	if (type2) {
	    unlen = Getc();
	    unlen += (Getc() << 8);
	    lzwflag = (unlen & 0x8000) ? 1 : 0;	/* flag is hi bit */
	    unlen &= 0x1fff;			/* strip extra stuff */
	    rleflag = (unlen != BLKSIZ);
	    if (lzwflag) {	/* will the real length bytes please stand up*/
		complen = Getc();
		complen += (Getc() << 8);
	    }
	} else {
	    unlen = Getc();
	    unlen += (Getc() << 8);
	    lzwflag = Getc();
	    rleflag = (unlen != BLKSIZ);
	}
#ifdef DEBUG1
	printf("Length after RLE = %d ($%.4x)\n", unlen, unlen);
	printf("LZW flag = %d, RLE flag = %d\n", lzwflag, rleflag);
	if (lzwflag != 0 && lzwflag != 1) {  /* this is weird... */
	    for (lzwflag = -6; lzwflag < 3; lzwflag++) {
		printf("foo %d: %.2x\n", lzwflag, *(ibuf+lzwflag));
	    }
	}
	if (type2 && lzwflag) {
	    printf("Length after RLE+LZW = %d ($%.4x)\n", complen, complen);
	}
#endif

	/* If it looks like we're going to run out of room, shift & read
	/* Mostly a guess; LZW length is less than unlen...  This is
	/* complicated and very prone to errors (but we err on the safe side).
	/* tmp4 is the number of bytes between the current ptr and the end;
	/* some (16-bit) compilers yack if it's all one statement.*/
	tmp4 = (fourbyt) buffer + (fourbyt) PAKBUFSIZ;
	tmp4 -= (fourbyt) ibuf;
	if (tmp4 < (unlen + 6)) {  /* 6 = 3/4 byte header + two just in case */
	    still_in_buf = tmp4;

#ifdef DEBUG1
	    printf("--- unlen = %d, space left = %d bytes\n",
		    unlen, still_in_buf);
#endif
/*	    BCopy((onebyt *) ibuf, (onebyt *) buffer, still_in_buf, FALSE);*/
	    BCOPY((onebyt *) ibuf, (onebyt *) buffer, still_in_buf);

	    if (comp_thread_eof != (fourbyt) 0) {  /* no read, just shift */
		if (comp_thread_eof > ((fourbyt) PAKBUFSIZ - still_in_buf)){
		    toread = (unsigned int) PAKBUFSIZ - still_in_buf;
		    comp_thread_eof -= (fourbyt) PAKBUFSIZ - still_in_buf;
		} else {
		    toread = (unsigned int) comp_thread_eof;
		    comp_thread_eof = (fourbyt) 0;
		}
#ifdef DEBUG1
		printf("--- reading another %u bytes\n", toread);
#endif
		if (read(srcfd, buffer+still_in_buf, toread) < toread)
		    Fatal("Unable to read [middle]", procName);
		if (verbose) Spin();
	    }
	    ibuf = buffer;
	}
    
	/* how much of the buffered data do we really need? */
	if (thread_eof > (fourbyt) BLKSIZ) {
	    partial = (unsigned int) BLKSIZ;
	    thread_eof -= (fourbyt) BLKSIZ;
	} else {
	    partial = (unsigned int) thread_eof;  /* last block of file */
	    thread_eof = (fourbyt) 0;
	}

	/*
	 * undo_LZW reads from ibuf (using Getc()) and writes to lbuf
	 * undo_LZW_2 does what undo_LZW does, but for LZW-II.
	 * undo_RLE reads from where you tell it and writes to rbuf
	 *
	 * This is really insane...
	 */
	if (lzwflag && rleflag) {
	    if (type2)
		undo_LZW_2(lbuf, unlen);  /* from ibuf -> lbuf */
	    else
		undo_LZW(lbuf, unlen);  /* from ibuf -> lbuf */
	    undo_RLE(lbuf, rbuf);  /* from lbuf -> rbuf */
	    wrbuf = rbuf;  /* write rbuf */
	} else if (lzwflag && !rleflag) {
	    if (type2)
		undo_LZW_2(lbuf, unlen);  /* from ibuf -> lbuf */
	    else
		undo_LZW(lbuf, unlen);  /* from ibuf -> lbuf */
	    wrbuf = lbuf;  /* write lbuf */
	} else if (!lzwflag && rleflag) {
	    undo_RLE(ibuf, rbuf);  /* from ibuf -> rbuf */
	    wrbuf = rbuf;  /* write rbuf */
	    ibuf += unlen;  /* have to skip over RLE-only data */
			/* normally ibuf is advanced by Getc() calls */
	    Table = Real_tab-256;   /* must clear table if no LZW */
	    entry = 0x101;  /* start at $101 */
	    reset_fix = 0;

	} else {
	    wrbuf = ibuf;  /* write ibuf */
	    ibuf += partial;  /* skip over uncompressed data */
			/* normally ibuf is advanced by Getc() calls */
	    Table = Real_tab-256;   /* must clear table if no LZW */
	    entry = 0x101;  /* start at $101 */
	    reset_fix = 0;
	}

	if (type2)
	    CRC = CalcCRC(CRC, wrbuf, partial);
	else
	    CRC = CalcCRC(CRC, wrbuf, BLKSIZ);

#ifdef DEBUG1
	printf("Writing %d bytes.\n", partial);
#endif
	if (crlf(dstfd, wrbuf, partial) < partial)  /* write wrbuf */
	    Fatal("Bad write", procName);
    }

    if (CRC != blkCRC) {
	fprintf(stderr, "WARNING: CRC does not match...");
	if (verbose) fprintf(stderr, "CRC is %.4x vs %.4x\n", CRC, blkCRC);
	else fprintf(stderr, "extract with V suboption to see filenames.\n");
    }
}

