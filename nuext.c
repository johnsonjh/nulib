/*
 * nuext.c - operations which extract from a NuFX archive
 *
 * NuLib v3.2  March 1992  Freeware (distribute, don't sell)
 * By Andy McFadden (fadden@uts.amdahl.com)
 */
#ifdef APW
segment "NuMain"
#endif

#include "nudefs.h"
#include <stdio.h>
#ifdef BSD43
# include <strings.h>
#else  /* SYSV, APW, MSC */
# include <string.h>
#endif
#include <fcntl.h>

#ifdef UNIX
# include <errno.h>
# include <time.h>
# include <sys/types.h>
# include <sys/stat.h>
#endif
#ifdef APW
# include <types.h>
# include <prodos.h>
# include <shell.h>
# include <strings.h>
#endif
#ifdef MSDOS
# include <io.h>
# include <time.h>
# include <stdlib.h>
# include <errno.h>
# include <direct.h>
# include <utime.h>	/* need <sys/utime.h> under MSC6? */
# include <sys/types.h>
# include <sys/stat.h>
#endif

#include "nuread.h"
#include "nuext.h"
#include "nupak.h"
#include "nuetc.h"

static BOOLEAN extall;	/* extract all files? */
static BOOLEAN print;   /* extract to screen rather than file? */


/*
 * Get the answer to a yes/no question.
 *
 * Returns TRUE for yes, FALSE for no.  May return additional things in the
 * future... (y/n/q)?
 */
int
AskYesNo()
{
    char buf[16];  /* if user answers with >16 chars, bad things happen */
    char c;

    printf(" (y/n)? ");
    fflush(stdout);
    gets(buf);
    c = *buf;
    if ((c == 'y') || (c == 'Y'))
	return (TRUE);
    else
	return (FALSE);
}


/*
 * Convert a filename to one legal in the present file system.
 *
 * Does not allocate new space; alters string in place (so original string
 * will be "corrupted").  Assumes that it has been passed a filename without
 * the filename separators.
 */
void
ConvFileName(str)
char *str;
{
    int idx = 0;
#ifdef UNIX

    while (*str != '\0') {
	*str &= 0x7f;  /* clear hi bit */
	if (*str == '/') *str = '.';
	if (++idx > 255) { *str = '\0'; break; }	/* MAXNAMELEN? */
	str++;
    }
#else
# ifdef APW
    static char *legal =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.";

    /* assumes ProDOS limits, not GS/OS */
    *str &= 0x7f;
    if ( ((*str < 'A') && (*str > 'Z'))  ||  ((*str < 'a') && (*str > 'z')) )
	*str = 'X';  /* must start with alpha char */
    while (*str != '\0') {
	if (!INDEX(legal, *str)) *str = '.';
	if (++idx > 15) { *str = '\0'; break; }
	str++;
    }
# endif /* APW */
# ifdef MSDOS
    char *ostr = str, *prev_dot = NULL;

    while (*str != '\0') {
       *str &= 0x7f;  /* clear hi bit */
       if (*str == '/') *str = '_';
       if (*str == '\\') *str = '_';
       if (*str == '!') *str = '_';
       if (*str == ':') *str = '_';
       if (*str == '.') {
	   if (prev_dot != NULL) *prev_dot = '_';
	   prev_dot = str;
	}
       if (++idx > 255) { *str = '\0'; break; }
       str++;
    }
    /* now limit the chars before the '.' to 8, and after to 3 */
    /* (if no dot, cut it at 8) */
    if (prev_dot == NULL) {
	if (strlen(str) > 8) str[8] = '\0';
    } else {
	*prev_dot = '\0';
	if (strlen(prev_dot+1) > 3) *(prev_dot+4) = '\0';
	if (strlen(ostr) > 8) {
	    *prev_dot = '.';
	    while (*prev_dot)
		*((ostr++) + 8) = *(prev_dot++);
	    *((ostr++) + 8) = *(prev_dot++);
	} else
	    *prev_dot = '.';
    }
# endif /* MSDOS */

# ifndef APW
# ifndef MSDOS
    printf("Need [other] filename converter\n");  /* +PORT+ */
# endif /* none2 */
# endif /* none1 */
#endif /*UNIX*/
}

/*
 * Set a file's attributes according to info in a record structure.
 */
void
SetFInfo(filename, RHptr)
char *filename;
RHblock *RHptr;
{
    static char *procName = "SetFInfo";
#ifdef UNIX
    long ltime;
    time_t timep[2];

    ltime = ReduceTime(&RHptr->mod_when);  /* set both to mod time */
    timep[0] = ltime;  /* accessed */
    timep[1] = ltime;  /* modified */
    utime(filename, timep);

    if ((RHptr->access == 0xe3L) || (RHptr->access == 0xc3L))  /* unlocked */
	chmod(filename, S_IREAD | S_IWRITE | 044);
    if ((RHptr->access == 0x21L) || (RHptr->access == 0x01L))  /* locked */
	chmod(filename, S_IREAD | 044);

#else  /* UNIX */
# ifdef APW
    /*
     * Call ProDOS SET_FILE_INFO to set attributes for a file.
     * Uses the information in the record header block.
     */
    FileRec finfo;
    OpenRec oinfo;
    twobyt date, time;
    long ltime;

    finfo.pathname = c2pstr(filename);	/* temp storage...? */
    finfo.fAccess = (twobyt) RHptr->access;
    finfo.fileType = (twobyt) RHptr->file_type;
    finfo.auxType = RHptr->extra_type;
    finfo.storageType = 0;  /* RHptr->storage_type otherwise */
    ltime = ReduceTime(&RHptr->create_when);
    date = (twobyt) ltime;  /* date is lower 16 */
    time = (twobyt) (ltime >> 16);  /* time is upper */
    finfo.createDate = date;
    finfo.createTime = time;
    ltime = ReduceTime(&RHptr->mod_when);
    date = (twobyt) ltime;  /* date is lower 16 */
    time = (twobyt) (ltime >> 16);  /* time is upper */
    finfo.modDate = date;
    finfo.modTime = time;

    SET_FILE_INFO( &finfo );
    ToolErrChk();
# endif /* APW */
# ifdef MSDOS
    long ltime;
    time_t timep[2];

    ltime = ReduceTime(&RHptr->mod_when);
    timep[0] = ltime;  /* accessed */
    timep[1] = ltime;  /* modified */
    utime(filename, timep);

    if ((RHptr->access == 0xe3L) || (RHptr->access == 0xc3L))  /* unlocked */
       chmod(filename, S_IREAD | S_IWRITE | 044);
    if ((RHptr->access == 0x21L) || (RHptr->access == 0x01L))  /* locked */
       chmod(filename, S_IREAD | 044);
# endif /* MSDOS */

# ifndef APW
# ifndef MSDOS
    printf("need [other] SetFInfo stuff\n");  /* +PORT+ */
# endif /* none2 */
# endif /* none1 */
#endif /* APW */
}


/*
 * Create a subdirectory
 *
 * This routine will exit on most errors, since generally more than one file
 * will be unpacked to a given subdirectory, and we don't want it charging
 * bravely onward if it's going to run into the same problem every time.
 */
void
CreateSubdir(pathname)
char *pathname;
{
    char *buffer = (char *) Malloc(MAXFILENAME+6);
    static char *procName = "CreateSubdir";
#ifdef UNIX
    struct stat st;

    /* if no directory exists, then make one */
    if (stat(pathname, &st) < 0)
	if (errno == ENOENT) {
	    sprintf(buffer, "mkdir %s", pathname);
	    if (system(buffer) != 0)  /* call UNIX mkdir to create subdir */
		Fatal("Unable to create subdir", procName);
	} else {
	    Fatal("Unable to create dir", procName);
	}
#else
# ifdef APW
    static FileRec create_p = { "", 0x00e3, 0x000f, 0L, 0x000d, 0, 0 }; /*dir*/
    FileRec info_p;  /* check if file exists, is dir */
    int err;  /* holds _toolErr */

    strcpy(buffer, pathname);
    c2pstr(buffer);
    info_p.pathname = buffer;
    GET_FILE_INFO( &info_p );

    switch (_toolErr) {
    case 0x0000:  /* no error */
	if (info_p.storageType != 0x000d)  /* not a DIR? */
	    Fatal("File in path exists, is not a directory.", procName);
	return;  /* file exists, is directory, no need to create */

    case fileNotFound:
	create_p.pathname = buffer;
	CREATE( &create_p );
	if   (!_toolErr) return;   /* created okay? */
	else ToolErrChk();

    default:  /* unknown error */
	ToolErrChk();
	Fatal("whoops!", procName); /* shouldn't get here */
    }
# endif /* APW */
# ifdef MSDOS
    struct stat st;

    /* if no directory exists, then make one */
    if (stat(pathname, &st) < 0)
	if (errno == ENOENT) {
	    if (mkdir(pathname) != 0)
		Fatal("Unable to create subdir", procName);
        } else {
	    Fatal("Unable to create dir", procName);
        }
# endif /* MSDOS */

# ifndef APW
# ifndef MSDOS

    /* don't forget to check if it exists first... */  /* +PORT+ */
    printf("don't know how to create [other] subdirectories\n"); /* mkdir() */
# endif /* none2 */
# endif /* none1 */
#endif /* UNIX */
    free(buffer);
}


/*
 * Given a pathname, create subdirectories as needed.  All file names are run
 * through a system-dependent filename filter, which means that the pathname
 * has to be broken down, the subdirectory created, and then the pathname
 * reconstructed with the "legal" pathname.  The converted filename is held
 * in a static buffer; subsequent calls will overwrite the previous string.
 *
 * This is useful when unpacking "dir1/dir2/fubar" and dir1 and dir2 don't
 * necessarily exist.
 *
 * It is assumed that all filenames are relative to the current directory.
 * According to the NuFX docs (revision 3 2/3/89), initial separators (like
 * "/", "\", or ":") should NOT be included.  If they are, this routine may
 * break.
 */
static char *
CreatePath(pathname, fssep)
char *pathname;  /* full pathname; should not include ProDOS volume name */
onebyt fssep;  /* file system pathname separator, usually "/" or "\" */
{
    int idx;
    char *ptr;
    static char workbuf[MAXFILENAME];  /* work buffer; must be static */
    static char *procName = "CreatePath";

    idx = 0;
    while (TRUE) {  /* move through string */
	ptr = INDEX(pathname, fssep);	/* find break */
	if (ptr)  /* down to actual filename? */
	    *ptr = '\0';  /* no, isolate this part of the string */

	strcpy(&workbuf[idx], pathname);  /* copy component to buf */
	ConvFileName(&workbuf[idx]); /* convert to legal str; may be shorter */
	idx += strlen(&workbuf[idx]);  /* advance index to end of string */
	if (!ptr) {  /* down to actual filename? */
	    workbuf[idx] = '\0';  /* yes, clean up */
	    break;  /* out of while */
	}
	workbuf[idx] = '\0';

	CreateSubdir(workbuf);	/* system-dependent dir create */

#ifdef UNIX
	workbuf[idx++] = '/';	/* tack a filename separator on, and advance */
	*ptr = '/';	/* be nice */
#else
# ifdef APW
	workbuf[idx++] = '/';
	*ptr = '/';
# endif
# ifdef MSDOS
	workbuf[idx++] = '\\';	/* was '\' */
	*ptr = '\\';		/* (ditto) */
# endif
# ifndef APW		/* +PORT+ */
# ifndef MSDOS
	workbuf[idx++] = '/';
	*ptr = '/';
# endif
# endif
#endif /*UNIX*/

/* was:	workbuf[idx++] = fssep;  /* tack an fssep on the end, and advance */
/* was:	*ptr = fssep;	/* be nice */

	pathname = ptr+1;  /* go again with next component */
    }

    return (workbuf);
}


/*
 * Extract a thread, and place in a file.
 *
 * Returns TRUE if the extract was successful, FALSE otherwise.  The most
 * common reason for a FALSE return value is a "no" answer when asked about
 * overwriting an existing file.
 */
static BOOLEAN
ExtractThread(arcfd, fileposn, destpn, THptr)
int arcfd;	/* source file descriptor (must be open) */
long fileposn;	/* position of data in source file */
char *destpn;	/* destination filename */
THblock *THptr;  /* pointer to thread info */
{
    int dstfd;	/* destination file descriptor */
    static char *procName = "ExtractThread";

    if (!print) {
	if (Exists(destpn)) {
	    if (interact) {
		if (verbose) printf("file exists, overwite");
		else         printf("%s exists, overwite", destpn);
		if (!AskYesNo()) {  /* return w/o overwriting */
		    return (FALSE);
		}
	    }
	    if (verbose) { printf("overwriting..."); fflush(stdout); }
	    if (unlink(destpn) < 0)
		Fatal("Unable to remove existing file", procName);
	}

	if ((dstfd =
		open(destpn, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, (mode_t)WPERMS)) < 0)
	    Fatal("Unable to open target path", procName);

	if (lseek(arcfd, (off_t) fileposn, S_ABS) < 0)
	    Fatal("Seek failed", procName);

	if (!UnpackFile(arcfd, dstfd, THptr,
			dopack ? THptr->thread_format : 0, pakbuf)) {
	    if (close(dstfd) < 0)
		Fatal("Dest close failed", procName);
	    unlink(destpn);  /* some sys can't delete while file open */
	} else {
	    if (close(dstfd) < 0)
		Fatal("Dest close failed", procName);
	}

    } else {  /* print */
	if ((dstfd = fileno(stdout)) < 0)
	    Fatal("Unable to get file for stdout", procName);
	if (lseek(arcfd, (off_t) fileposn, S_ABS) < 0)
	    Fatal("Seek failed", procName);

	if (!UnpackFile(arcfd, dstfd, THptr,
	    		dopack ? THptr->thread_format : 0, pakbuf)) {
	    printf("Unpack failed.\n");
	    return (FALSE);
	}
	fflush(stdout);
    }

    return (TRUE);
}


/*
 * Handle message_threads
 */
static void
message_thread(arcfd, RNodePtr, TNodePtr)
int arcfd;
RNode *RNodePtr;
TNode *TNodePtr;
{
    int i;
    int oldTo, oldFrom;
    static char *procName = "message_thread";

    switch (TNodePtr->THptr->thread_kind) {
    case 0x0000:  /* ASCII text */
	printf("Found obsolete ASCII text thread (ignored)\n");
	break;
    case 0x0001:  /* ASCII text, predefined size */
	if (verbose && !print && TNodePtr->THptr->thread_eof) {
	    printf("\n--- Comment for file '%s':\n", RNodePtr->filename);
	    fflush(stdout);
	    if (lseek(arcfd, (off_t) TNodePtr->fileposn, S_ABS) < 0)
		Fatal("unable to seek to comment", procName);
	    oldTo = transto;
	    oldFrom = transfrom;
	    transto = -1;	/* switch to CR -> current mode */
	    transfrom = 0;	/* (assumes created under ProDOS) */
				/* may need to fix this later (but how?) */
	    FCopy(arcfd, fileno(stdout), TNodePtr->THptr->thread_eof,
		pakbuf, TRUE);
#ifdef FUBAR
	    print = TRUE;
	    verbose = FALSE;  /* turn off "unshrinking..." messages */
	    ExtractThread(arcfd,TNodePtr->fileposn, "stdout", TNodePtr->THptr);
	    print = FALSE;
	    verbose = TRUE;
#endif
	    transto = oldTo;
	    transfrom = oldFrom;
	    putchar('\n');
	}
	break;
    case 0x0002:  /* standard Apple IIgs icon */
	printf("Found Apple IIgs Icon thread (ignored)\n");
	break;
    default:
	printf("Found unknown message_thread %.4x in '%s'\n",
	    TNodePtr->THptr->thread_kind, RNodePtr->filename);
	break;
    }
}

/*
 * Handle control_threads
 */
static void
control_thread(arcfd, RNodePtr, TNodePtr)
int arcfd;
RNode *RNodePtr;
TNode *TNodePtr;
{
    switch (TNodePtr->THptr->thread_kind) {
    case 0x000:  /* create dir */
	printf("Found create directory control thread (ignored)\n");
	break;
    default:
	printf("Found unknown control_thread %.4x in '%s'\n",
	    TNodePtr->THptr->thread_kind, RNodePtr->filename);
	break;
    }
}


/*
 * Handle data_threads
 *
 * Does not guarantee that the archive file position is anything rational;
 * the TNode's fileposn should be (and is) used here.
 */
static void
data_thread(arcfd, RNodePtr, TNodePtr)
int arcfd;
RNode *RNodePtr;
TNode *TNodePtr;
{
    long fileposn;     /* absolute position of thread in file */
    long old_eof;
    char *fn;
    int ov;

    if (print)  /* this is something of a hack... */
	if (TNodePtr->THptr->thread_kind != 0x0000) {  /* not a data fork? */
	    fprintf(stderr, "Can't print non-data fork for '%s'.\n",
				RNodePtr->filename);
	    return;  /* this hoses the file posn... */
	} else {
	    if (verbose) printf("\n*****  %s  *****\n", RNodePtr->filename);
	    fflush(stdout);
	    ov = verbose;
	    verbose = FALSE;  /* turn off "unshrinking..." messages */
	    fileposn = TNodePtr->fileposn;
	    ExtractThread(arcfd,fileposn, "stdout", TNodePtr->THptr);
	    verbose = ov;
	    return;
	}

    switch (TNodePtr->THptr->thread_kind) {
    case 0x0000:  /* data fork */
	if (verbose) {
	    printf("Extracting '%s' (data)...", RNodePtr->filename);
	    fflush(stdout);
	}

	/* create any needed subdirs */
	fn = CreatePath(RNodePtr->filename, RNodePtr->RHptr->file_sys_info);

	/* extract the file */
	if (ExtractThread(arcfd, TNodePtr->fileposn, fn, TNodePtr->THptr)) {
	    SetFInfo(fn, RNodePtr->RHptr);  /* set file attributes, dates... */
	    if (verbose) printf("done.\n");
	}
	break;

    case 0x0001:  /* disk image */
/*	printf("Found disk image (not extracted)\n");*/

	if (verbose) {
	    printf("Extracting '%s' (disk image)...", RNodePtr->filename);
	    fflush(stdout);
	}

	/* setup path (normalize file name) */
	fn = CreatePath(RNodePtr->filename, RNodePtr->RHptr->file_sys_info);

	/* thread_eof is invalid for disks, so figure it out */
	old_eof = TNodePtr->THptr->thread_eof;
	if (RNodePtr->RHptr->storage_type <= 3) {	/* should be block */
	    TNodePtr->THptr->thread_eof =		/* size, but shk301 */
		RNodePtr->RHptr->extra_type * 512;	/* stored it wrong */
	} else {
	    TNodePtr->THptr->thread_eof =
		RNodePtr->RHptr->extra_type * RNodePtr->RHptr->storage_type;
	}

	/* extract the disk into a file */
	if (ExtractThread(arcfd, TNodePtr->fileposn, fn, TNodePtr->THptr)) {
	    /*SetFInfo(fn, RNodePtr->RHptr);/* set file attributes, dates... */
	    if (verbose) printf("done.\n");
	}

	TNodePtr->THptr->thread_eof = old_eof;
	break;

    case 0x0002:  /* resource_fork */
	printf("Found resource_fork (not extracted)\n");
	break;
    default:
	printf("Found unknown data_thread %.4x in '%s'\n",
	    TNodePtr->THptr->thread_kind, RNodePtr->filename);
	break;
    }
}


/*
 * Extract files from archive
 *
 * Scan archive, extracting files which start with the strings in "names".
 * Calls subroutines to handle the various thread_class types.
 */
static void
Extract(filename, namecount, names)
char *filename;
int namecount;
char **names;
{
    ListHdr *archive;
    int arcfd;	/* archive file descriptor */
    int rec, idx;
    MHblock *MHptr;   /* Master Header block */
    RNode *RNodePtr;  /* Record Node */
    TNode *TNodePtr;   /* Thread block */
    int len, *lentab;  /* hold strlen() of all names */
    char *pn;  /* archived pathname */
    int thread;  /* current thread #; max 65535 threads */
    BOOLEAN gotone = FALSE;
    static char *procName = "Extract";

    archive = NuRead(filename);
    if ((arcfd = open(archive->arc_name, O_RDONLY | O_BINARY)) < 0)
	Fatal("Unable to open archive", procName);

    pakbuf = (onebyt *) Malloc(PAKBUFSIZ);  /* allocate unpack buffer */

    if (!namecount) {
	extall = TRUE;
	lentab = (int *) NULL;
    } else {
	lentab = (int *) Malloc(sizeof(int) * namecount);
	for (idx = 0; idx < namecount; idx++)  /* calc. once (for efficiency) */
	    lentab[idx] = strlen(names[idx]);
    }

    MHptr = archive->MHptr;
    RNodePtr = archive->RNodePtr;

    /* main record read loop */
    for (rec = 0; rec < MHptr->total_records; rec++) {
	pn = RNodePtr->filename;
	len = strlen(pn);
	if (RNodePtr->RHptr->version_number > MAXVERS) {
	    printf("Unable to extract '%s': unknown record version_number\n",
		    pn);
	    continue;  /* with for */
	}

	for (idx = 0; extall || idx < namecount; idx++) { /* find arced file */
	    /* try to match argument with first few chars of stored filename */
	    /* or the entire filename, depending on EXPAND flag */
	    if (extall || ((len >= lentab[idx]) && doExpand ?
			(!strncasecmp(pn, names[idx], lentab[idx])) :
			(!strcasecmp(pn, names[idx])) )) {

		gotone = TRUE;
		/* go through all threads */
		TNodePtr = RNodePtr->TNodePtr;
		for (thread = 0; thread < (int) RNodePtr->RHptr->total_threads;
								   thread++) {
		    switch(TNodePtr->THptr->thread_class) {
		    case 0x0000:
			message_thread(arcfd, RNodePtr, TNodePtr);
			break;
		    case 0x0001:
			control_thread(arcfd, RNodePtr, TNodePtr);
			break;
		    case 0x0002:
			/* don't extract if doMessages is set */
			if (!doMessages)
			    data_thread(arcfd, RNodePtr, TNodePtr);
			break;
		    case 0x0003:
			/* filename_thread; ignore */
			break;
		    default:
			printf("Unknown thread_class %.4x for '%s'\n",
			    TNodePtr->THptr->thread_class, RNodePtr->filename);
			break;
		    }
		    TNodePtr = TNodePtr->TNext;
		}
		break;	/* out of filename matching (inner) FOR loop */
	    }
	}

	RNodePtr = RNodePtr->RNext;  /* move on to next record */
    }
    if (!gotone && verbose)
	printf("None selected\n");
    if (close(arcfd) < 0)
	Fatal("Source (archive) close failed", procName);

}

/*
 * Entry point to extract routines.
 */
void
NuExtract(filename, namecount, names, options)
char *filename;
int namecount;
char **names;
char *options;
{
    static char *procName = "NuExtract";

    if (*options == 'p') {  /* printing rather then extracting to file */
	print = TRUE;
	dopack = TRUE;  /* no extract uncompressed! */
    } else print = FALSE;

    Extract(filename, namecount, names);  /* do stuff */
}

