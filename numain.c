/*
 * numain.c - shell-based front end for NuLib
 *
 * NuLib v3.2  March 1992  Freeware (distribute, don't sell)
 * By Andy McFadden (fadden@uts.amdahl.com)
 */
#ifdef APW
segment "main"
#endif

static char *header =
 "NuLib v3.24  Januray 1993  Freeware   Copyright 1989-93 By Andy McFadden";

#include "nudefs.h"	/* system-dependent defines */
#include "stdio.h"	/* standard I/O library */
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>	/* C type stuff, like tolower() */
#ifdef BSD43
# include <strings.h>
#else  /* SYSV, APW, MSC */
# include <string.h>	/* string stuff */
#endif

#ifdef APW
# include <stdlib.h>
# include <types.h>
# include <strings.h>
# include <shell.h>
#endif

#include "nuread.h"	/* structs for archive info, archive read routines */
#include "nuview.h"	/* archive listing functions */
#include "nuadd.h"	/* archive operations (add, create, move) */
#include "nuext.h"	/* archive operations (extract) */
#include "nupdel.h"	/* archive operations (delete, update, freshen) */
#include "nublu.h"	/* Binary II archive operations */
#include "nupak.h"	/* need PAKBUFSIZ */	
#include "nuetc.h"	/* Malloc(), Fatal(), etc. */

extern char *getenv();  /* +PORT+ */

#define Whoops(str) printf("WARNING: typedef %s may be set incorrectly\n",str);
#define ENVAR	"NULIBOPT"  /* environment variable with options in it */

/*
 * global to entire program
 */
int HiLo;		/* byte ordering; FALSE on low-first (65816) */
int verbose;		/* print verbose? */
int interact;		/* interactive overwrite mode? */
int dopack;		/* do we want to pack/unpack? */
int doExpand;		/* do we want to expand archive filenames? */
int doSubdir;		/* process subdirectories at all? */
int doMessages;		/* do comments instead of data */
int transfrom;	/* how to do CR<->LF translation (from what?)  (-1 = off) */
int transto;	/* translate to ? */
int packMethod;		/* how to pack a file */
int diskData;		/* store files as if they are disk images */
fourbyt defFileType;	/* default file type */
fourbyt defAuxType;	/* default aux type */
onebyt *pakbuf;  /* used by compression routines; created once to reduce */
		 /* overhead involved in malloc()ing a 64K buffer */
char *prgName = "NuLib";    /* for error messages; don't like argv[0] */
			    /* besides, the name changes every 3 weeks */

/*
 * Print simple usage info
 */
static void
Usage(argv0)
char *argv0;
{
    printf("\nUsage: %s option[suboptions] archive-name [filespec]\n", argv0);
    printf("\nType \"%s h\" for help.\n", argv0);
}


/*
 * Print usage info
 */
static void
Help(argv0, options)
char *argv0, *options;
{
    if (INDEX(options+1, 'n')) {  /* using 'n' suboption? */
	printf("%s\n", header);
	printf("\nCompression methods:\n");
	printf("  #  Name                        Abbr  Pack?  Unpack?\n");
	printf("  0: Uncompressed                unc     Y      Y\n");
	printf("  1: SQueezed (sq/usq)           squ     N      Y\n");
	printf("  2: Dynamic LZW-I (ShrinkIt)    shk     Y      Y\n");
	printf("  3: Dynamic LZW-II (ShrinkIt)   sh2     N      Y\n");
	printf("  4: UNIX 12-bit compress        u12     Y      Y\n");
	printf("  5: UNIX 16-bit compress        u16     Y      Y\n");
	printf("The default is #2\n");
	printf("\nText conversion methods (during extraction):\n");
	printf("  0: Convert from CR to this system (ProDOS files)\n");
	printf("  1: Convert from LF to this system (UNIX files)\n");
	printf("  2: Convert from CRLF to this system (MS-DOS files)\n");

    } else if (INDEX(options+1, 'w')) {  /* print author info */

	printf("%s\n", header);
	printf(
       "Internet: fadden@uts.amdahl.com   Usenet: ...!amdahl!fadden\n");
	printf("\nShrinkIt and NuFX standard by Andy Nicholas.\n");
	printf(
  "ShrinkIt LZW compression by Kent Dickey.  LZW-II (a modified version of\n");
	printf(
 "  Kent's algorithm) designed by Andy Nicholas.  C LZW-II decoder by Kent\n");
	printf(
	  "  Dickey and Frank Petroski (independently and simultaneously).\n");
	printf("\nUNIX compress code adapted from COMPRESS v4.3.\n");
	printf(
"\nBinary II unpack and unsqueeze C code adapted from unblu.c and usq.c by\n");
	printf("  Marcel J.E. Mol (usq.c based on usq2/sq3 by Don Elton).\n");
	printf("\nMS-DOS port by Robert B. Hess and Bruce Kahn.\n");
	printf(
"\nThis program is Freeware.  Please distribute as widely as possible, but\n");
	printf(
      "  don't sell it.  Source code is available via e-mail upon request.\n");
	printf(
	   "\nUsers without Usenet/Internet access may send mail to me at:\n");
	printf("  1474 Saskatchewan Drive\n");
	printf("  Sunnyvale, CA 94087\n");

    } else if (INDEX(options+1, 's')) {  /* suboption info */
	printf("%s\n", header);
	printf("\nUsual meaning of suboptions:\n");
	printf("  c  - compression type, followed by a number\n");
	printf(
	    "  f  - file/aux type to add, followed by file/aux type spec\n");
	printf("  d  - store the files, but mark them as disk images\n");
	printf("  i  - interactive; prompt before overwriting file\n");
	printf("  m  - messages (add/extract comments instead of data)\n");
	printf("  r  - don't recursively descend subdirectories\n");
	printf("  s  - storage type (store as compressed w/o compressing), ");
	printf("followed by number\n");
	printf(
	   "  t  - text translation (CR<->LF), followed by conversion mode\n");
	printf("  u  - store as uncompressed (same as c0)\n");
	printf("  v  - verbose mode\n");
#ifndef NO_BLU
	printf("  x  - extract during Binary II operations\n");
#endif
	printf("  +  - match partial pathnames for extract and delete\n");
	printf("\nTable of contents suboptions:\n");
	printf("  v  - verbose output (same as V option)\n");
	printf("  a  - ARC/ZOO style format\n");
	printf("  z  - full output, prints all aspects of archive\n");

	printf("\nSample shell variable command (csh):\n");
	printf(
      "  setenv NULIBOPT=verbose,interactive,type=SRC,aux=000a,compress=5\n");
	printf("  (default is non-verbose, non-interactive, type=NON, ");
	printf(  "aux=0000, compress=2)\n");
	
    } else {  /* default help screen */

	printf("%s\n", header);
	printf(
	     "\nUsage: %s option[suboptions] archive-name [filespec]\n", argv0);
	printf("Option must be one of:\n");
	printf("  a[vucsrfd]  add to archive\n");
#ifndef NO_BLU
	printf("  b[xvti]     Binary II archive operations\n");
#endif
	printf(
       "  c[vucsrfd]  create archive (add, but suppress 'create' message)\n");
	printf("  d[v+]       delete file from archive\n");
	printf("  f[vucsrfd]  freshen files in archive\n");
	printf("  h[snw]      show help screen (subopt/numbers/who's who)\n");
	printf("  i[v]        verify archive integrity\n");
	printf("  m[vucsrfd]  move files to archive (add, delete original)\n");
	printf("  p[vmt+]     print archived file to stdout\n");
	printf("  t[vaz]      display table of contents\n");
	printf("  u[vucsrfd]  update files in archive\n");
	printf("  v           verbose listing (ProDOS 8 ShrinkIt format)\n");
	printf("  x,e[vumti+] extract from archive\n");
    }
}


/*
 * Check machine dependencies
 */
static void
CheckMach()
{
    onebyt one;
    onebyt *oneptr;
    twobyt two;
    fourbyt four;
    /*
     * If you get an error on either of these lines, then you need to
     * specify definitions for them in nudefs.h (see the comments in the
     * file for more info).
     */
    off_t off;
    mode_t mode;

#ifdef UNIX
# ifdef APW
    ^^ "ERROR: You have both APW and UNIX defined" ^^
# endif
# ifdef MSDOS
    ^^ "ERROR: You have both MSDOS and UNIX defined" ^^
# endif
#endif /*UNIX*/
#ifdef APW
# ifdef MSDOS
    ^^ "ERROR: You have both APW and MSDOS defined" ^^
# endif
#endif

    /* some compilers complain about (unsigned) -1 , so I'm doing it this */
    /* way to keep everybody quiet.					  */

    one = 0x100;
    if (one)
	Whoops("onebyt");  /* one > 1 */
    two = 0x10000;
    if (two)
	Whoops("twobyt");  /* two > 2 */
    two = 0x1000;
    if (!two)
	Whoops("twobyt");  /* two < 2 */
    four = 0xffffffff;
    four++;
    if (four)
	Whoops("fourbyt");  /* four > 4 */
    four = 0x10000;
    if (!four)
	Whoops("fourbyt");  /* four < 4 */

    /* check byte ordering */
    two = 0x1122;
    oneptr = (onebyt *) &two;
    if (*oneptr == 0x11)
	HiLo = TRUE;
    else if (*oneptr == 0x22)
	HiLo = FALSE;
    else {
	printf("WARNING: Unable to determine a value for HiLo\n");
	HiLo = FALSE;
    }

    /* check some other stuff... compilers may (should?) give warnings here */
    if (ATTSIZE < MHsize)
	printf("WARNING: ATTSIZE must be >= than MHsize\n");
    if (RECBUFSIZ < ATTSIZE)
	printf("WARNING: RECBUFSIZ should be larger than ATTSIZE\n");
    if (MHsize != 48 || THsize != 16)
	printf("WARNING: Bad MHsize or THsize\n");
    if (sizeof(Time) != 8)
	printf("WARNING: struct Time not 8 bytes\n");
}


/*
 * Check to see if string 2 is in string 1.
 *
 * Returns the position of string 2 within string 1; -1 if not found.
 */
static int
strc(host, sub)
char *host, *sub;
{
    int hlen = strlen(host);
    int slen = strlen(sub);
    int i;

    if (slen > hlen)  /* substring longer than host string */
	return (-1);

    /* generic linear search... */
    for (i = 0; i <= (hlen - slen); i++)
	if ((*(host+i) == *sub) && (!strncmp(host+i, sub, slen)))
	    return (i);
    
    return (-1);
}


/*
 * Yank a number from a character string.
 */
int
OptNum(ptr)
char *ptr;
{
    int val = 0;

    while (*ptr && isdigit(*ptr)) {
	val *= 10;
	val += (*ptr - '0');
	ptr++;
    }
    return (val);
}


/*
 * Set default values for globals.
 *
 * Should be of form "NULIBOPT=..."
 *   verbose		: default to verbose output
 *   interactive	: default to interactive mode when overwriting
 *   type=xxx		: set storage type to ProDOS type "xxx"
 *   aux=xxxx		: set aux storage type to 4-byte hex number "xxxx"
 */
void GetDefaults(options)
char *options;
{
    char *envptr;
    int off, idx, pt;
    int len = strlen(options);
    char type[5];

    /* set program default values */
    verbose = FALSE;	/* silent mode */
    interact = FALSE;	/* don't ask questions */
    doSubdir = TRUE;	/* process subdirectories unless told not to */
    dopack = TRUE;	/* don't pack unless told to */
    doExpand = FALSE;	/* don't expand archived filenames */
    doMessages = FALSE;	/* do comments instead of data */
    packMethod = 0x0002;/* ShrinkIt LZW */
    diskData = FALSE;	/* store as ordinary files */
    transfrom = -1;	/* no text translation */
    transto = -1;
    defFileType = (fourbyt) 0;	/* NON */
    defAuxType = (fourbyt) 0;	/* $0000 */

    /* read from global envir var */
    if (envptr = getenv(ENVAR)) {
	if (strc(envptr, "verbose") >= 0) {
	    verbose = TRUE;
	}
	if (strc(envptr, "interactive") >= 0) {
	    interact = TRUE;
	}
	if ((off = strc(envptr, "compress=")) >= 0) {
	    off += 9;
	    if (off+1 > strlen(envptr)) {
		fprintf(stderr, "Error with 'compress=n' in NULIBOPT var\n");
		Quit (-1);
	    }
	    packMethod = atoi(envptr+off);
	}
	if ((off = strc(envptr, "type=")) >= 0) {
	    off += 5;
	    if (off+3 > strlen(envptr)) {
		fprintf(stderr, "Error with 'type=xxx' in NULIBOPT var\n");
		Quit (-1);
	    }
	    strncpy(type, envptr+off, 3);
	    type[3] = '\0';
	    for (idx = 0; idx < 256; idx++)  /* scan for file type */
		if (!strcasecmp(FT[idx], type)) {
		    defFileType = (fourbyt) idx;
		    break;  /* out of for */
		}
	}
	if ((off = strc(envptr, "aux=")) >= 0) {
	    off += 4;
	    if (off+4 > strlen(envptr)) {
		fprintf(stderr, "Error with 'aux=$xxxx' in NULIBOPT var\n");
		Quit (-1);
	    }
	    strncpy(type, envptr+off, 4);
	    type[4] = '\0';
	    sscanf(type, "%x", &defAuxType);
	}
    }

    /* handle command line suboption string */
    for (pt = 1; pt < len; pt++) {  /* skip option char */
	switch(options[pt]) {
	case '+':	/* expand */
	    doExpand = TRUE;
	    break;
	case 'a':	/* ARC/ZOO output format */
	    /* do nothing */
	    break;
	case 'c':	/* compress method */
	    packMethod = OptNum(&options[pt+1]);
	    while (pt < len && isdigit(options[pt+1]))  /* advance to next */
		pt++;
	    dopack = TRUE;
	    break;
	case 'd':	/* treat as disk images instead of ordinary files */
	    diskData = TRUE;
	    break;
	case 'f':	/* filetype specified */
	    strncpy(type, &options[pt+1], 3);
	    type[3] = '\0';
	    for (idx = 0; idx < 256; idx++)  /* scan for file type */
		if (!strcasecmp(FT[idx], type)) {
		    defFileType = (fourbyt) idx;
		    break;  /* out of for */
		}

	    pt += strlen(type);
	    if (options[pt+1] == '/') {  /* auxtype specification */
		pt++;
		strncpy(type, &options[pt+1], 4);
		type[4] = '\0';
		sscanf(type, "%lx", &defAuxType);
		pt += strlen(type);
	    }
	    break;
	case 'i':	/* interactive overwrites */
	    interact = TRUE;
	    break;
	case 'm':	/* do messages instead of data */
	    doMessages = TRUE;
	    break;
	case 'n':	/* help with numbers */
	    /* do nothing */
	    break;
	case 'r':	/* don't recursively descend subdir */
	    doSubdir = FALSE;
	    break;
	case 's':	/* store method */
	    packMethod = OptNum(&options[pt+1]);
	    while (pt < len && isdigit(options[pt+1]))  /* advance to next */
		pt++;
	    dopack = FALSE;
	    break;
	case 't':	/* how to translate text? */
	    transfrom = OptNum(&options[pt+1]);
	    while (pt < len && isdigit(options[pt+1]))
		pt++;
	    break;
	case 'u':	/* don't use compression */
	    dopack = FALSE;  /* this doesn't matter, but FALSE may be faster */
	    packMethod = 0x0000;  /* archive w/o compression */
	    break;
	case 'v':	/* verbose mode */
	    verbose = TRUE;
	    break;
	case 'w':	/* help on people */
	    /* do nothing */
	    break;
	case 'x':	/* extract BLU files */
	    /* do nothing */
	    break;
	case 'z':	/* in view files */
	    /* do nothing */
	    break;
	default:	/* unknown */
	    fprintf(stderr, "%s: unknown subopt '%c'\n", prgName, options[pt]);
	    break;  /* do nothing */
	}
    }  /* for */
}


#ifdef APW
/*
 * Expand a ProDOS filename using APW wildcard calls (even if the file doesn't
 * exist).
 *
 * Returns a pointer to a buffer holding the filename.
 */
char *
ExpandFilename(filename)
char *filename;
{
    char *ptr;

    c2pstr(filename);
    if (!(*filename)) {
	printf("Internal error: can't expand null filename\n");
	Quit (-1);
    }

    INIT_WILDCARD(filename, 0);
    ToolErrChk();
    p2cstr(filename);

    NEXT_WILDCARD(tmpNameBuf);
    p2cstr(tmpNameBuf);
    if (strlen(tmpNameBuf))  /* found it */
	return(tmpNameBuf);
    else {
	/* file does not exist; expand path */
	strcpy(tmpNameBuf, filename);
	ptr = RINDEX(tmpNameBuf, '/');	/* remove filename */
	if (!ptr)  /* filename only */
	    return (filename);

	*ptr = '\0';
	if (!strlen(tmpNameBuf)) {  /* something weird... */
	    printf("Unable to expand '%s'\n", filename);
	    Quit (-1);
	}

	c2pstr(tmpNameBuf);
	INIT_WILDCARD(tmpNameBuf, 0);
	ToolErrChk();

	NEXT_WILDCARD(tmpNameBuf);
	p2cstr(tmpNameBuf);
	if (!strlen(tmpNameBuf))  {
	    printf("Unable to fully expand '%s'\n", filename);
	    Quit (-1);
	}

	strcat(tmpNameBuf, RINDEX(filename, '/'));
	return (tmpNameBuf);
    }
}
#endif /* APW */


/*
 * Parse args, call functions.
 */
main(argc, argv)
int argc;
char **argv;
{
    char *filename;  /* hold expanded archive file name */
    int idx;

    filename = (char *) Malloc(MAXFILENAME);
    CheckMach();  /* check compiler options, and set HiLo */

    if (argc < 2) {  /* no arguments supplied */
	Usage(argv[0]);
	Quit (0);
    }

    if (argv[1][0] == '-') {  /* skip initial dashes */
	argv[1]++;
    }
    for (idx = 0; argv[1][idx]; idx++)	/* conv opts to lower case */
	if (isupper(argv[1][idx]))
	    argv[1][idx] = tolower(argv[1][idx]);

    if (argc < 3) {  /* no archive file specified; show help screen */
	if (argv[1][0] == 'h')  /* could be HS, HN, or HW */
	    Help(argv[0], argv[1]);
	else			/* not 'H' option; show generic help scrn */
	    Help(argv[0], "h");
	Quit (0);
    }

#ifdef APW
    strcpy(filename, ExpandFilename(argv[2]));
#else
    strcpy(filename, argv[2]);
#endif
    GetDefaults(argv[1]);  /* get defaults, process suboption string */

    pakbuf = (onebyt *) Malloc(PAKBUFSIZ);  /* allocate global pack buf */
    switch (argv[1][0]) {
    case 'a':  /* add */
    case 'c':  /* create */
    case 'm':  /* move */
	NuAdd(filename, argc-3, argv+3, argv[1]);  /* NuAdd will read */
	break;
#ifndef NO_BLU
    case 'b':  /* Binary II operations */
	NuBNY(filename, argc-3, argv+3, argv[1]);
	break;
#endif
    case 'd':  /* delete */
	NuDelete(filename, argc-3, argv+3, argv[1]);
	break;
    case 'f':  /* freshen */
    case 'u':  /* update */
	NuUpdate(filename, argc-3, argv+3, argv[1]);
	break;
    case 'i':  /* verify integrity */
	NuTest(filename, argv[1]);
	break;
    case 't':  /* table of contents */
    case 'v':  /* verbose output */
	NuView(filename, argv[1]);
	break;
    case 'e':  /* extract */
    case 'x':
    case 'p':
	NuExtract(filename, argc-3, argv+3, argv[1]);
	break;
    default:   /* need help */
	fprintf(stderr, "%s: unknown option '%c'\n", argv[0], argv[1][0]);
	break;
    }

    free (filename);
    free (pakbuf);
    Quit (0);
}

