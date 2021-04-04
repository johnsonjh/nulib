/*
 * nudefs.h - system-dependent typdefs, and global #defines and variables.
 *
 * NuLib v3.2  March 1992  Freeware (distribute, don't sell)
 * By Andy McFadden (fadden@uts.amdahl.com)
 */
/*
 * IMPORTANT: This file must be first on the list of #includes, since some
 *	      include files will be processed based on these #defines
 */

/* SYSTEM DEPENDENCIES */
typedef unsigned char onebyt;
typedef unsigned short twobyt;
typedef unsigned long fourbyt;

/* byte ordering; TRUE if high byte is first (68xxx), else FALSE (65xxx) */
extern int HiLo;  /* actually part of numain.c */

/* Setup for LINUX machines */
#define UNIX
#define SYSV
#define NO_RENAME

/* Setup for Apple //gs APW */
/* [ "APW" is automatically defined by the compiler ] */

/* Setup for MS-DOS machines (80xxx based: IBM PC) */
/* #ifndef MSDOS */
/* #define MSDOS */
/* #endif */

/* Setup for AIX */
/* #define SYSV */
/* #define BSD_INCLUDES */

/* Setup for BSD UNIX systems */
/*#define UNIX*/
/*#define BSD43*/

/* Setup for the NeXT */
/* #define UNIX */
/* #define BSD43 */
/* #define NeXT */		/* (is this defined automatically?) */

/* Setup for XENIX/386 */
/* NOTE: if you get error messages about readdir() and opendir() during */
/*	 linking, remove the leading '#' from the line "#CLIBS= -lx" in */
/*	 "Makefile"							*/
/* #define XENIX386 */
/* #define UNIX */
/* #define SYSV */
/* #define NO_RENAME */           /* no rename() call in C library */

/* Setup for Amdahl UTS 2.1; also works on the AT&T 3B2 */
/* #define UNIX */
/* #define SYSV */
/* #define NO_RENAME */
/* (UTS only:) */
/* #define HAS_EFT */    /*(don't forget to add "-eft" flag in Makefile for UTS 2.1)*/

/* Setup for AOS/VS @ DG */
/* #define UNIX */
/* #define DATAGENERAL */
/* #define AOSVS */

/* Setup for AViiONs */
/* #define UNIX */
/* #define SYSV */
/* #define HAS_EFT */

/* Setup for other UNIX systems */
/* #define UNIX */
/* #define SYSV (or whatever) */

/* use table lookups to get CRCs */
#define CRC_TAB

/* don't include Binary II */
/* #define NO_BLU */

/* don't include UNIX compress */
/* #define NO_UCOMP */


#ifdef UNIX
/* (the #include setup for NuLib is pretty screwed up at this point...) */
# include <sys/types.h>		/* need off_t, if it exists */

/*
 * With EFT, off_t is a longlong (8 bytes) and mode_t is four bytes (ulong).
 * They affect lseek() and open() calls.  Without EFT, off_t is usually
 * four bytes, and mode_t is a short or an int.
 *
 * I'm not sure which systems don't have off_t or mode_t.  BSD seems to have
 * off_t, but not mode_t; this'll have to be handled on a case-by-case basis.
 * Unfortunately #ifdefs don't pick up typedefs on all compilers...
 */
# ifndef HAS_EFT
/*typedef long off_t;*/
/*typedef unsigned short mode_t;*/
# endif

#else
# ifndef HAS_EFT
/* APW and MS-DOS should be the same */
typedef long off_t;
typedef unsigned int mode_t;
# endif
#endif

extern off_t lseek();		/*VERY important if lseek() doesn't return int*/


/*
 * The rest of this stuff shouldn't need to be changed
 */

/*
 * Some global defs
 */

/* errno wasn't defined in <errno.h> on some systems... */
#ifndef MSDOS
extern int errno;
#endif

/* Maximum file name length that we intend to handle (watch stack size!) */
#define MAXFILENAME	   1024

/* file operations */
#define S_ABS	0  /* seek absolute */
#define S_REL	1  /* seek relative */
#define S_END	2  /* seek from end */

#ifdef UNIX	      /* stuff for open() */
# define WPERMS 0644	/* read/write for owner, read only for others */
# define FREAD_STR	"r"
# define FWRITE_STR	"w"
# define O_BINARY 0    /* for non-UNIX open(); easier than #ifdefs */
#else
# ifdef APW
#  define WPERMS 0666	 /* read/write for all; this may not work for some */
#  define FREAD_STR	"rb"
#  define FWRITE_STR	"wb"
# endif
# ifdef MSDOS
#  define S_IREAD     0000400         /* read permission, owner */
#  define S_IWRITE    0000200         /* write permission, owner */
#  define WPERMS S_IREAD | S_IWRITE
#  define FREAD_STR	"rb"
#  define FWRITE_STR	"wb"
# endif
# ifndef WPERMS /* other system */
#  define WPERMS 0666  /* +PORT+ */
#  define FREAD_STR	"rb"
#  define FWRITE_STR	"wb"
# endif
#endif /*UNIX*/

/* Time structure; same as TimeRec from misctool.h */
/* one-byte entries should not have alignment problems... */
typedef struct {
    onebyt second;
    onebyt minute;
    onebyt hour;
    onebyt year;
    onebyt day;
    onebyt month;
    onebyt extra;
    onebyt weekDay;
} Time;


/*
 * global to entire program
 */
extern int HiLo;	/* byte ordering; FALSE on low-first (65816) */
extern int verbose;	/* BOOLEAN: print verbose? */
extern int interact;	/* BOOLEAN: interactive when overwriting? */
extern int dopack;	/* BOOLEAN: do we want to pack/unpack? */
extern int doSubdir;	/* BOOLEAN: expand subdirectories? */
extern int doExpand;	/* BOOLEAN: expand archived filenames? */
extern int doMessages;	/* BOOLEAN: do comments instead of data? */
extern int transfrom;	/* how to do CR<->LF translation?  (-1 = none) */
extern int transto;
extern int packMethod;	/* how to pack a file (thread_format) */
extern int diskData;	/* BOOLEAN: store files as disk images */
extern fourbyt defFileType;	/* default file type */
extern fourbyt defAuxType;	/* default aux type */
extern onebyt *pakbuf;	/* used by compression routines; created once to */
              /* eliminate overhead involved in malloc()ing a 64K buffer */
extern char *prgName;	/* for errors; don't like argv[0] */

