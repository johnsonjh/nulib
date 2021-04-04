/*
 * nuetc.c - extra stuff; mostly system-dependent subroutines.
 *
 * NuLib v3.2  March 1992  Freeware (distribute, don't sell)
 * By Andy McFadden (fadden@uts.amdahl.com)
 */
#ifdef APW
segment "NuMain"
#endif

#include "nudefs.h"
#include "stdio.h"
#include <fcntl.h>
#include <errno.h>	  /* errno declarations */
#include <ctype.h>	  /* for tolower(), isupper() */

#ifdef UNIX
# include <time.h>	  /* need localtime() */
# include <sys/types.h>   /* defn of time_t */
# include <sys/stat.h>
#endif
#ifdef APW
# include <stdlib.h>	   /* exit(), etc. */
# include <types.h>	   /* has _toolErr in it */
# include <prodos.h>
# include "apwerr.h"	   /* APW/ProDOS error codes */
#endif
#ifdef MSDOS
# include <process.h>
# include <stdlib.h>
# include <io.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <time.h>
#endif

#include "nuetc.h"
/* note "nuread.h" is not included... none of the routines here assume */
/* any knowledge of NuFX archives.                                     */

#ifndef MSDOS
extern char *malloc(); /* all systems ... except DOS : RBH */
#endif  /* +PORT+ */

extern void free();
extern char *getenv();
#ifdef APW
extern Time ReadTimeHex();  /* should be TimeRec, from misctool.h */
#endif

/* This is a generally available TEMPORARY filename buffer */
char tmpNameBuf[MAXFILENAME];


/******************** general misc routines ********************/

/*
 * Fatal error handler
 */
void
Fatal(deathstr, procName)
char *deathstr, *procName;
{
    fflush(stdout);
    fprintf(stderr, "\n%s: fatal error: %s\n--- ", prgName, deathstr);
    perror(procName);
    Quit (-1);
}


/*
 * Quit can be used to perform cleanup operations before exiting.
 */
void
Quit(val)
int val;
{
    exit(val);
}


/*
 * Safe malloc()... checks return value
 */
char *
Malloc(size)
int size;
{
    char *ptr = (char *) malloc(size);

    if (ptr != (char *) NULL) {
	return(ptr);
    } else {
	/* 8910.31 - RBH: report byte size that failed */
	printf("Malloc: memory alloc error [%u : bytes]\n", size);
#ifdef MSDOS
	/* (this doesn't work under Turbo C - MSC only) */
	printf("(Largest Available Block: %u)\n", _memmax());
#endif
	Quit (-1);
	/*NOTREACHED*/
    }
}

/******************** UNIX compatibility routines ********************/

#ifdef UNIX
/*
 * Convert expanded date to a number of seconds for UNIX systems.
 *
 * Remember that *atime follows the NuFX docs for time values, which
 * don't necessarily match those in UNIX manual pages.
 * Adapted from _Advanced UNIX Programming_ by Marc J. Rochkind.
 *
 * Returns 0 if date/time is invalid.
 */
long
timecvt(atime)
Time *atime;
{
    long tm;
    int days;
    BOOLEAN isleapyear;
    int tzone;
    char *getenv(), *tz;

    if ((tz = getenv("TZ")) == NULL)
	tzone = 8;  /* pacific std time */
    else
	tzone = atoi(&tz[3]);

    isleapyear = (atime->year != 0) && (atime->year%4 == 0);  /* 2000 isn't */
    if (atime->year < 70)
	atime->year += 100;  /* years after 2000 */
    days = (atime->year - 70) * 365L;
    days += ((atime->year - 69L) / 4);  /* previous years' leap days */

    switch (atime->month +1) {  /* month is 0-11 */
    case 12:
	days += 30;  /* Nov */
    case 11:
	days += 31;  /* Oct */
    case 10:
	days += 30;  /* Sep */
    case 9:
	days += 31;  /* Aug */
    case 8:
	days += 31;  /* Jul */
    case 7:
	days += 30;  /* Jun */
    case 6:
	days += 31;  /* May */
    case 5:
	days += 30;  /* Apr */
    case 4:
	days += 31;  /* Mar */
    case 3:
	days += (isleapyear ? 29 : 28);  /* Feb */
    case 2:
	days += 31;  /* Jan */
    case 1:
	break;
    default:
	/*printf("Invalid month\n");*/
	return (0L);
    }

    if (atime->day > 31) {
	/*printf("Invalid day\n");*/
	return (0L);
    }
    tm = (days + atime->day) * 24L * 60L * 60L;

    if (atime->hour > 23) {
	/*printf("Invalid hour\n");*/
	return (0L);
    }
    atime->hour += tzone;  /* correct for time zone */
    tm += atime->hour * 60L * 60L;

    if (atime->minute > 59) {
	/*printf("Invalid minute\n");*/
	return (0L);
    }
    tm += atime->minute * 60L;

    if (atime->second > 59) {
	/*printf("Invalid second\n");*/
	return (0L);
    }
    tm += atime->second;

    if (localtime(&tm)->tm_isdst)  /* was that day in dst? */
	tm -= 60L * 60L;  /* adjust for daylight savings */
    return (tm);
}
#endif /* UNIX */

/******************** APW compatibility routines ********************/

#ifdef APW
/*
 * Normally a C library function to print out a description of the most
 * recent system (non-toolbox, non-ProDOS) error.  Exists under UNIX and
 * MS C 5.1, so I'm assuming it exists most everywhere else...
 */
void
perror(errstr)
char *errstr;
{
    fflush(stdout);
    if ( (errno > 0) && (errno < sys_nerr) ) {	/* known APW error? */
	fprintf(stderr, "%s: %s\n", errstr, sys_errlist[errno]);
    } else {
	fprintf(stderr, "%s: ", errstr);
	fflush(stderr);
	ERROR( errno );
    }
    Quit (-1);
}


/* Check for //gs toolbox errors; all are fatal */
void
ToolErrChk()
{
    int err = _toolErr;

    if (err) {
	if (err < MPErr) {  /* was a ProDOS error? */
	    fprintf(stderr, "Error: $%.2x %s\n", (char) err,
		ProDOSErr[err]);
	} else {
	    fprintf(stderr, "Tool err ($%.4x): ", err);
	    fflush(stderr);
	    ERROR( err );
	}
	Quit (-1);
    }
}

#endif /* APW */

/******************** miscellaneous string routines ********************/

/*
 * Compare strings, ignoring case (may be in standard C lib; stricmp()?)
 */
int
strcasecmp(str1, str2)
register char *str1, *str2;
{
    register char one, two;
    register int val;

    for ( ; *str1 && *str2; str1++, str2++) {
	one = (isupper(*str1) ? tolower(*str1) : *str1);
	two = (isupper(*str2) ? tolower(*str2) : *str2);
	if (val = two - one)
	    return (val);
    }
    if (!(*str1) && !(*str2))  /* both zero -> equivalent */
	return (0);
    else {  /* one is shorter; return result */
	one = (isupper(*str1) ? tolower(*str1) : *str1);
	two = (isupper(*str2) ? tolower(*str2) : *str2);
	return (two - one);
    }
}

int
strncasecmp(str1, str2, num)
register char *str1, *str2;
int num;
{
    register int i;
    register char one, two;
    register int val;  /* keep going 'til no more registers... */

    for (i = 0; (i < num) && (*str1) && (*str2); i++, str1++, str2++) {
	one = (isupper(*str1) ? tolower(*str1) : *str1);
	two = (isupper(*str2) ? tolower(*str2) : *str2);
	if (val = two - one)
	    return (val);
    }
    if (i == num)  /* first num characters are equal, so return zero */
	return (0);
    else {  /* one ended early; return result */
	one = (isupper(*str1) ? tolower(*str1) : *str1);
	two = (isupper(*str2) ? tolower(*str2) : *str2);
	return (two - one);
    }
}

/******************* file-related routines ********************/

/*
 * Do operating system-dependent CREATE stuff
 *
 * Creates a NuFX archive file, with type info where necessary.
 * Does not leave file open.
 */
void
ArcfiCreate(filename)
char *filename;
{
    static char *procName = "ArcfiCreate";
#ifdef UNIX
    int fd;

    if ((fd = open(filename, O_CREAT|O_RDWR, (mode_t) WPERMS)) < 0)
	Fatal("Unable to create file", procName);
    close(fd);
#else
# ifdef APW
    FileRec create_p;

    c2pstr(filename);
    create_p.pathname = filename;
    create_p.fAccess = 0x00e3;
    create_p.fileType = 0x00e0;  /* LBR */
    create_p.auxType = 0x8002;	 /* SHK */
    create_p.storageType = 0x0001;
    create_p.createDate = 0x0000;  /* let ProDOS fill in the blanks */
    create_p.createTime = 0x0000;
    CREATE( &create_p );
    ToolErrChk();
    p2cstr(filename);
# endif /* APW */
# ifdef MSDOS
    int fd;

    if ((fd = open(filename, O_CREAT|O_RDWR, (mode_t) WPERMS)) < 0)
	Fatal("Unable to create file", procName);
    close(fd);
# endif /* MSDOS */

# ifndef APW
# ifndef MSDOS
    int fd;

    if ((fd = open(filename, O_CREAT|O_RDWR, (mode_t) WPERMS)) < 0)
	Fatal("Unable to create file", procName);
    close(fd);
# endif /* none2 */
# endif /* none1 */
#endif /* UNIX */
}


/*
 * Determine if a file already exists.
 */
BOOLEAN
Exists(filename)
char *filename;
{
    static char *procName = "Exists";
#ifdef UNIX
    struct stat sm;

    if (stat(filename, &sm) < 0) {
	if (errno == ENOENT)  /* if doesn't exist, then okay */
	    return (FALSE);
	else {  /* some other problem killed stat(), probably serious */
	    fprintf(stderr, "Unable to stat() '%s'\n", filename);
	    Fatal("Bad stat()", procName); /*serious prob*/
	    /*NOTREACHED*/
	}
    } else  /* successful call - file exists */
	return (TRUE);
#else
# ifdef APW
    FileRec info_p;  /* check if file exists, is dir */
    int err;

    c2pstr(filename);
    info_p.pathname = filename;
    GET_FILE_INFO( &info_p );
    err = _toolErr;
    p2cstr(filename);
    if (err == pathNotFound || err == fileNotFound)
	return (FALSE);
    else if (!err)
	return (TRUE);
    else {
	_toolErr = err;
	ToolErrChk();
	return (TRUE);
    }
# endif /* APW */
# ifdef MSDOS
    struct stat sm;

    if (stat(filename, &sm) < 0) {
       if (errno == ENOENT)  /* if doesn't exist, then okay */
           return (FALSE);
       else  /* some other problem killed stat(), probably serious */
           fprintf(stderr, "Unable to stat() '%s'\n", filename);
           Fatal("Bad stat()", procName); /*serious prob*/
    } else  /* successful call - file exists */
       return (TRUE);
# endif /* MSDOS */

# ifndef APW
# ifndef MSDOS
    printf("Need [other] Exists()\n");  /* +PORT+ */
    return (FALSE);
# endif /* none2 */
# endif /* none1 */
#endif /* UNIX */
}


/*
 * Generate a temporary file name (system dependent).
 * Assumes space is allocated for buffer.
 */
char *
MakeTemp(buffer)
char *buffer;
{
    static char *procName = "MakeTemp";
#ifdef UNIX
    extern char *mktemp();

    strcpy(buffer, "nulb.tmpXXXXXX");
    return (mktemp(buffer));
#else
# ifdef APW
    int idx = 0;

    do {
        sprintf(buffer, "nulb.tmp%d", idx++);
    } while (Exists(buffer));
    return (buffer);
# endif /* APW */
# ifdef MSDOS
    extern char *mktemp();

    strcpy(buffer, "nulbXXXX.tmp");
    return (mktemp(buffer));
# endif /* MSDOS */

# ifndef APW
# ifndef MSDOS
    strcpy(buffer, "nulb.tmp");  /* +PORT+ */
    return (buffer);
# endif /* none2 */
# endif /* none1 */
#endif /* UNIX */
}

#ifdef NO_RENAME
/*
 * This is a replacement for the library call, in case somebody's C library
 * doesn't have it.
 */
int
rename(fromname, toname)
{
    if (link(fromname, toname) < 0)
	return (-1);
    if (unlink(fromname) < 0)
	return (-1);
}
#endif


/*
 * Rename a file.
 */
void
Rename(fromname, toname)
char *fromname, *toname;
{
    static char *procName = "Rename";
#ifdef UNIX
    if (Exists(toname)) {
	fprintf(stderr, "\n%s: WARNING: Unable to rename '%s' as '%s'\n",
	    prgName, fromname, toname);
	fflush(stderr);
    }
# ifdef AOSVS					/* BAK 04/30/90 */
    printf("Work on AOS/VS rename command\n");	/* BAK 04/30/90 */
# else						/* BAK 04/30/90 */
    else {
	if (rename(fromname, toname) < 0) {  /* this should "never" fail */
	    fprintf(stderr,
		"\n%s: WARNING: Unable to rename '%s' as '%s'\n",
		prgName, fromname, toname);
	    Fatal("Bad rename()", procName); /*serious prob*/
	}
    }
# endif
#else
# ifdef APW
    PathNameRec cpath_p;

    if (Exists(toname)) {
	fprintf(stderr, "\n%s: WARNING: Unable to rename '%s' as '%s'\n",
	    prgName, fromname, toname);
	fflush(stderr);
	return;
    }

    cpath_p.pathname = fromname;
    cpath_p.newPathname = toname;
    c2pstr(fromname);
    c2pstr(toname);
    CHANGE_PATH( &cpath_p );
    ToolErrChk();
    p2cstr(fromname);
    p2cstr(toname);
# endif /* APW */
# ifdef MSDOS
    if (Exists(toname)) {
	fprintf(stderr, "\n%s: WARNING: Unable to rename '%s' as '%s'\n",
	    prgName, fromname, toname);
	fflush(stderr);
	return;
    }
    printf("Work on MSDOS rename command\n");
# endif /* MSDOS */

# ifndef APW
# ifndef MSDOS
    if (Exists(toname)) {
	fprintf(stderr, "\n%s: WARNING: Unable to rename '%s' as '%s'\n",
	    prgName, fromname, toname);
	fflush(stderr);
	return;
    }
    printf("Need [other] rename command\n");  /* +PORT+ */
# endif /* none2 */
# endif /* none1 */
#endif /*UNIX*/
}

/******************** date/time routines ********************/

/*
 * Expand date/time from file-sys dependent format to eight byte NuFX format.
 * tptr is filesys format, TimePtr is NuFX format
 */
void
ExpandTime(tptr, TimePtr)	   /* (BSD) UNIX version */
onebyt *tptr;  /* usually points to a time_t (long) */
Time *TimePtr;
{
#ifdef UNIX
    time_t *tp = (time_t *) tptr;
    struct tm *unixt;

    unixt = localtime(tp);  /* expand time_t into components */
    TimePtr->second = unixt->tm_sec;
    TimePtr->minute = unixt->tm_min;
    TimePtr->hour = unixt->tm_hour;
    TimePtr->year = unixt->tm_year;
    TimePtr->day = unixt->tm_mday -1;  /* want 0-xx, not 1-xx */
    TimePtr->month = unixt->tm_mon;
    TimePtr->extra = 0;
    TimePtr->weekDay = unixt->tm_wday +1;  /* Sunday = 1, not 0 like UNIX */
#else
# ifdef APW			    /* APW version */
    twobyt date, time;

    date = (twobyt)tptr[0] + ((twobyt)tptr[1] << 8);
    time = (twobyt)tptr[2] + ((twobyt)tptr[3] << 8);
    TimePtr->second = 0;  /* not stored in ProDOS file info */
    TimePtr->minute = (char) time;  /* truncated to char */
    TimePtr->hour = time >> 8;
    TimePtr->year = date >> 9;
    TimePtr->day = (date & 0x1f) - 1;
    TimePtr->month = ((date & 0x01e0) >> 5) - 1;
    TimePtr->extra = 0;
    TimePtr->weekDay = 0;
# endif /* APW */
# ifdef MSDOS
    struct tm *newtime;
    time_t *tp = (time_t *) tptr;

    newtime = localtime (tp);
    TimePtr->second = (onebyt)newtime->tm_sec;
    TimePtr->minute = (onebyt)newtime->tm_min;
    TimePtr->hour   = (onebyt)newtime->tm_hour;
    TimePtr->year   = (onebyt)newtime->tm_year;
    TimePtr->day    = (onebyt)newtime->tm_mday - 1;
    TimePtr->month  = (onebyt)newtime->tm_mon;
    TimePtr->extra  = 0;
    TimePtr->weekDay= (onebyt)newtime->tm_wday + 1;
# endif /* MSDOS */

# ifndef APW
# ifndef MSDOS
    printf("Need [other] time-expander\n");  /* +PORT+ */
    TimePtr->second = 0;
    TimePtr->minute = 0;
    TimePtr->hour = 0;
    TimePtr->year = 0;
    TimePtr->day = 0;
    TimePtr->month = 0;
    TimePtr->extra = 0;
    TimePtr->weekDay = 0;
# endif /* none1 */
# endif /* none2 */
#endif /* UNIX */
}


/*
 * Get current time, put in struct
 */
Time *
GetTime()
{
    static Time t;
#ifdef UNIX
    struct tm *unixt;
    time_t now = time(NULL);

    unixt = localtime(&now);
    t.second = unixt->tm_sec;
    t.minute = unixt->tm_min;
    t.hour = unixt->tm_hour;
    t.year = unixt->tm_year;
    t.day = unixt->tm_mday -1;	/* want 0-xx, not 1-xx */
    t.month = unixt->tm_mon;
    t.extra = 0;
    t.weekDay = unixt->tm_wday +1;  /* Sunday = 1, not 0 like UNIX */
    /* return (&t) */
#else
# ifdef APW
    t = ReadTimeHex(t);
    /* return (&t) */
# endif /* APW */
# ifdef MSDOS
    struct tm *pctime;
    time_t now = time(NULL);

    pctime = localtime(&now);
    t.second = (onebyt)pctime->tm_sec;
    t.minute = (onebyt)pctime->tm_min;
    t.hour   = (onebyt)pctime->tm_hour;
    t.year   = (onebyt)pctime->tm_year;
    t.day    = (onebyt)pctime->tm_mday -1; /* want 0-xx, not 1-xx */
    t.month  = (onebyt)pctime->tm_mon;
    t.extra  = 0;
    t.weekDay= (onebyt)pctime->tm_wday +1;  /* Sunday = 1, not 0 */
    /* return (&t) */
# endif /* MSDOS */

# ifndef APW
# ifndef MSDOS
    printf("\nNeed [other] GetTime\n");  /* +PORT+ */
    t->second = 0;
    t->minute = 0;
    t->hour = 0;
    t->year = 0;
    t->day = 0;
    t->month = 0;
    t->filler = 0;
    t->weekDay = 0;
    /* return (&t) */
# endif /* none1 */
# endif /* none2 */
#endif /* UNIX */
    return (&t);
}


/*
 * Convert a NuFX Time struct to a compact system-dependent format
 *
 * This is used to set a file's date when extracting.  Most systems don't
 * dedicate 8 bytes to storing the date; this reduces it to the format
 * used by a "set_file_date" command.
 */
long
ReduceTime(tptr)
Time *tptr;
{
#ifdef UNIX
    long t = timecvt(tptr);

    return (t ? t : time(NULL));	/* if stored time is invalid, */
					/* return current time */
#else
# ifdef APW
    twobyt date, time;
    long val;

    date = ((twobyt)tptr->year << 9) | ((((twobyt)tptr->month)+1) << 5) |
   	(((twobyt)tptr->day)+1);
    time = ((twobyt)tptr->hour << 8) | ((twobyt)tptr->minute);

    val = (long) date + ((long) time << 16);
    return (val);
# endif /* APW */
# ifdef MSDOS
    return (time(NULL)); /* not sure what to do, return current : RBH */
# endif /* MSDOS */

#ifndef APW
#ifndef MSDOS
    printf("Need [other] ReduceTime\n");  /* +PORT+ */
# endif /* none2 */
# endif /* none1 */
#endif /* UNIX */
}

