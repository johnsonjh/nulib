/*
 * nublu.c - operations on Binary II archives
 *
 * NuLib v3.2  March 1992  Freeware (distribute, don't sell)
 * By Andy McFadden (fadden@uts.amdahl.com)
 */
#ifdef APW
segment "Compress"
#endif

#ifndef NO_BLU                       /***********************************/

#include "nudefs.h"
#include "stdio.h"
#include <fcntl.h>

#ifdef UNIX
# include <errno.h>
# include <sys/types.h>
# include <sys/stat.h>
#endif
#ifdef APW
# include <prodos.h>
#endif
#ifdef MSDOS
# include <stdlib.h>
# include <io.h>
# include <string.h>
# include <sys/types.h>
# include <sys/stat.h>
#endif

#include "nuview.h"  /* file types for BLU */
#include "nuadd.h"   /* need OptNum() */
#include "nupak.h"   /* need unpak_SQU */
#include "nuetc.h"

/* Binary II extraction routines are adapted from:			*/
/*************************************************************************
 **									**
 **  Name    :	unblu							**
 **  Author  :	Marcel J.E. Mol 					**
 **  Date    :	10/05/88	      (first release)			**
 **  Version :	2.20							**
 **  Files   :	unblu.c 	Main source file			**
 **									**
 **  ------------------------- Revision List -------------------------	**
 **  Ver   Date       Name		     Remarks			**
 **  1.00  10/05/88   Marcel Mol	     Raw copy of a basic program**
 **  2.00  03/06/88   Marcel Mol	     Rewrite after blu info	**
 **					     was send to the net	**
 **  2.10  18/06/88   Marcel Mol	     Added filetype texts	**
 **  2.20  23/09/88   Marcel Mol	     Show mod and creation time **
 **									**
 ************************************************************************/


/*char * copyright = "@(#) unblu.c  2.1 18/06/88  (c) M.J.E. Mol";*/
#define BUFSIZE 128		    /* Blu block length */

/* global variables */
static char *progname;
static char *blufile;
static BOOLEAN extract = FALSE;  /* extract (as opposed to just listing) */


/*
 * extract_file -- extract file fname from the archive fd. Fname
 *		   contains filelen bytes.
 *
 * If the first block has the .QQ magic numbers, go ahead and try to
 * unsqueeze it.  Not the best way to go about it, but it works.
 */
static void
extract_file(fd, fname, filelen)
int fd;
char *fname;  /* 64 bytes */
long filelen;
{
    int ofd;
    int n, i;
    int j, len;
    onebyt buf[BUFSIZE];
    long full_len;
    int offset;
    static char *procName = "extract_file";

    /*n = */ read(fd, buf, 70);  /* read first few bytes */
    lseek(fd, (off_t) -70, S_REL);  /* back up */
    if ((buf[0] == 0x76) && (buf[1] == 0xff)) {  /* is it squeezed? */
	i = 0;				/* get the original file name */
	while ((fname[i] = buf[4+i]) != '\0')
	    i++;
	offset = 5+i;  /* how far into file is end of filename? */
	ConvFileName(fname);
	if (verbose) { printf("(as %s)...", fname);  fflush(stdout); }
    }

    ConvFileName(fname);	/* strip hi bits, adjust special chars, etc */
    len = strlen(fname);	/* (no longer used?) */

#ifdef FUBAR
    for (j = 0; j < len; j++)
	fname[j] &= 0x7f;	/* clear hi bits */
#endif

    if (Exists(fname)) {
	if (interact) {
	    if (verbose) printf("file exists, overwite");
	    else         printf("%s exists, overwite", fname);
	    if (!AskYesNo()) {  /* return w/o overwriting */
		full_len = ( (filelen / 128L) +1 ) * 128L;
		lseek(fd, (off_t) full_len, S_REL);
		return;
	    }
	}
	if (verbose) { printf("overwriting..."); fflush(stdout); }
	if (unlink(fname) < 0)
	    Fatal("Unable to remove existing file", procName);
    }

    if ((ofd = open(fname, O_BINARY|O_CREAT|O_WRONLY|O_TRUNC, (mode_t)0644))<0){
	Fatal("Can't open destination file", "extract_file");
    }

    if ((buf[0] == 0x76) && (buf[1] == 0xff)) {  /* is it squeezed? */
	if (verbose) { printf("unsqueezing...");  fflush(stdout); }
	full_len = ( (filelen / 128L) +1 ) * 128L;
	lseek(fd, (off_t) offset, S_REL);
	full_len -= offset;  /* send unpak_SQU everything past fname */

	unpak_SQU(fd, ofd, full_len);  /* unsqueeze it */

    } else {  /* extract uncompressed */

	lastseen = '\0';  /* used by crlf() translator */
	while (filelen > 0L) {
	    n = read(fd, buf, BUFSIZE);		/* Read 128 bytes */
	    if (n != BUFSIZE) {
		fprintf(stderr, "Extract_BNY: %s file size broken\n", blufile);
		Quit(-1);
	    }
	    if (crlf(ofd, buf, (filelen >= BUFSIZE ? BUFSIZE : filelen)) !=
			(filelen >= BUFSIZE ? BUFSIZE : filelen))
		Fatal("Bad write", procName);
	
	    filelen -= (long) BUFSIZE;
	}
    }
    close(ofd); 			     /* Close destination file */
}


/*
 * print_header -- print global information of the binary II file
 */
static void
print_header(buf)
onebyt *buf;
{
    long disk_blocks;

    disk_blocks = buf[117] + (buf[118]<<8) + (buf[119]<<16) + (buf[120]<<24);
    printf("Listing %-40.40s  ", blufile);
    printf("Blocks used: %-5ld", disk_blocks);
    printf("Files: %d\n", buf[127]+1);
    printf("\nFilename       Type Blocks   Modified        ");
    printf("Created           Length  Subtype\n\n");
}


/*
 * want -- return TRUE if name exists in array wantlist,
 *	   else return FALSE
 */
static BOOLEAN
want(name, wantlist)
char *name;
char **wantlist;
{
    while (*wantlist != NULL) {
	if (strcasecmp(name, *wantlist++) == 0)
	    return (TRUE);
    }
    return (FALSE);

}


/*
 * process_file -- retrieve or print file information of file given
 *		   in buf
 */
static void
process_file(fd, buf, count, wanted)
int  fd;
onebyt *buf;
int count;
char **wanted;
{
    int ftype, auxtype;
    int fnamelen;
    long filelen;
    char fname[64];
    char outbuf[16];  /* temp for sprintf */
    int nblocks, problocks;
    Time create_dt;
    Time mod_dt;
#ifdef APW
    FileRec frec;
#endif
#ifdef UNIX
    struct stat st;
#endif
#ifdef MSDOS
    struct stat st;
#endif
/* +PORT+ */
/*    int tf;
 *    int dflags;
 */
    static char *procName = "process_file";

    /* Get file info */
    ftype =  buf[4];				/* File type */
    auxtype = (int) buf[5] + ((int)buf[6] << 8);
    fnamelen =	buf[23];			/* filename */
    strncpy(fname, &buf[24], fnamelen);
    fname[fnamelen] = '\0';
    /* dflags =  buf[125]; 			/* Data flags */
    /* tf =  buf[127];				/* Number of files to follow */
    filelen = (long) buf[20] + ((long) buf[21] << 8) +
	    ((long) buf[22] << 16);	/* calculate file len */
    nblocks = (filelen + BUFSIZE-1) / BUFSIZE;	/* #of BNY blocks */
    problocks = buf[8] + ((int) buf[9] << 8);

    mod_dt.second = 0;
    mod_dt.minute = buf[12] & 0x3f;
    mod_dt.hour   = buf[13] & 0x1f;
    mod_dt.day	  = (buf[10] & 0x1f) -1;
    mod_dt.month  = (((buf[11] & 0x01) << 3) + (buf[10] >> 5)) -1;
    mod_dt.year   = buf[11] >> 1;
    mod_dt.weekDay= 0;
    create_dt.second = 0;
    create_dt.minute = buf[16] & 0x3f;
    create_dt.hour   = buf[17] & 0x1f;
    create_dt.day    = (buf[14] & 0x1f) -1;
    create_dt.month  = (((buf[15] & 0x01) << 3) + (buf[14] >> 5)) -1;
    create_dt.year   = buf[15] >> 1;
    create_dt.weekDay= 0;

    if (!count || want(fname, wanted)) {
	if (!extract) { /* print file information ONLY */
	    printf("%-15.15s %-3.3s ", fname, FT[ftype]);
	    printf("%6d  ", problocks);
	    printf("%-16.16s ", PrintDate(&mod_dt, TRUE));
	    printf("%-16.16s ", PrintDate(&create_dt, TRUE));
	    if (filelen < 0x100L)
		sprintf(outbuf, "$%.2lx", filelen);
	    else if (filelen < 0x10000L)
		sprintf(outbuf, "$%.4lx", filelen);
	    else sprintf(outbuf, "$%.6lx", filelen);
	    printf("%7s    ", outbuf);
	    printf("$%.4x\n", auxtype);

/*	    if (dflags == 0)
 *		printf("stored");
 *	    else {
 *		if (dflags & 128) {
 *		    printf("squeezed");
 *		}
 *		if (dflags & 64) {
 *		    printf("encrypted");
 *		}
 *		if (dflags & 1)
 *		    printf("packed");
 *	    }
 *	    putchar('\n');
 */
	    if (ftype != 15) {			/* If not a directory */
		lseek(fd, (off_t) BUFSIZE*nblocks, S_REL); /*Seek to next file*/
	    }

	} else {  /* extract is TRUE */

	    if (verbose) { printf("Extracting %s...", fname); fflush(stdout); }
#ifdef UNIX
	    if (ftype != 15)
		extract_file(fd, fname, filelen);  /* note dates etc not set */
	    else {
		/* if no directory exists, then make one */
		if (stat(fname, &st) < 0)
		    if (errno == ENOENT) {
			sprintf(tmpNameBuf, "mkdir %s", fname);
			if (system(tmpNameBuf) != 0)  /* call UNIX mkdir */
			    Fatal("Unable to create subdir", procName);
		    } else {
			Fatal("Unable to create dir", procName);
		    }
	    }
#else
# ifdef APW
	    /* create file/directory , with appropriate type/auxtype stuff */
	    c2pstr(fname);
	    frec.pathname = fname;
	    frec.fAccess = 0x00e3;  /* unlocked */
	    frec.fileType = ftype;
	    frec.auxType = (unsigned long) auxtype;
	    frec.storageType = (int) buf[7];
	    frec.createDate = 0x0000;  /* set later */
	    frec.createTime = 0x0000;

	    CREATE( &frec );
	    ToolErrChk();
	    p2cstr(fname);

	    extract_file(fd, fname, filelen);

	    /* set file attributes */
	    c2pstr(fname);
	    frec.fAccess = (word) buf[3];
	    frec.modDate = (word) buf[10] + ((word)buf[11] << 8);
	    frec.modTime = (word) buf[12] + ((word)buf[13] << 8);
	    frec.createDate = (word) buf[14] + ((word)buf[15] << 8);
	    frec.createTime = (word) buf[16] + ((word)buf[17] << 8);
	    SET_FILE_INFO( &frec );
	    ToolErrChk();
	    p2cstr(fname);
# else
	    if (ftype != 15)
		extract_file(fd, fname, filelen);
	    else  /* +PORT+ */
		printf("[ need [other] subdir create for UnBNY ]\n");
# endif /* APW */
#endif /* UNIX */
	    if (verbose) printf("done.\n");
	}
    } else if (ftype != 15) {	/* This file not wanted; if not a directory */
	lseek(fd, (off_t) BUFSIZE*nblocks, S_REL);       /* Seek to next file */
    }
}


/*
 * unblu -- process a binary II file fd, and process the filenames
 *	    listed in wanted. If wanted is \0, all files are processed.
 */
static void
unblu(fd, count, wanted)
int fd;
int count;
char **wanted;
{
    onebyt buf[BUFSIZE];
    int  firstblock = 1;	/* First block needs special processing */
    int  tofollow = 1;		/* Files to follow in the archive */
    int  n;

    while (tofollow && ((n = read(fd, buf, BUFSIZE)) > 0)) {
						/* If there is a header block */

	if (n != BUFSIZE) {
	    fprintf(stderr, "UnBNY: %s file size is broken\n", blufile);
	    Quit(-1);
	}
	if ((buf[0] != 10) || (buf[1] != 71) ||
	    (buf[2] != 76) || (buf[18] != 2)) {
	    fprintf(stderr,
		"UnBNY: %s not a Binary II file or bad record\n", blufile);
	    Quit(-1);
	}

	tofollow = buf[127];		/* How many files to follow */
	if (firstblock && !extract) {
	    print_header(buf);
	}
	firstblock = 0;
	process_file(fd, buf, count, wanted);	 /* process the file for it */

    }
    if (firstblock && (n < 0))  /* length < 128 */
	fprintf(stderr, "UnBNY: Not a Binary II file");
}


/*
 * Main entry point from CShrink
 */
void
NuBNY(filename, argc, argv, options)
char *filename;
int argc;
char **argv;
char *options;
{
    int  bfd;				/* File descriptor for blu file */
    char *optr;

    /* process X subopt ourselves */
    if (INDEX(options+1, 'x')) extract = TRUE;
    else extract = FALSE;

    blufile = filename; 	  /* Make it global */
    if ((bfd = open(filename, O_RDONLY | O_BINARY)) < 0)
	Fatal("Unable to open Binary II archive", "NuBNY");

    unblu(bfd, argc, argv);	     /* Process wanted files */

    close(bfd);
    Quit(0);
}

#endif /*NO_BLU*/                    /***********************************/

