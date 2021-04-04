/*
 * nuadd.c - operations which add to a NuFX archive
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
#include <errno.h>
#include <ctype.h>
#ifdef BSD43
# include <strings.h>
#else
# include <string.h>
#endif

#ifdef UNIX
# include <sys/types.h>
# include <sys/stat.h>
# ifdef SYSV
#  ifdef XENIX386
#   include <sys/ndir.h>	/* maybe <sys/ndir.h>, <dirent.h>, <dir.h>...*/
#  else /*other SYSV*/
#   include <dirent.h>
#  endif
# endif
# ifdef BSD43
#  include <sys/dir.h>
# endif
#endif

#ifdef APW
# include <types.h>
# include <prodos.h>
# include <shell.h>
# include <strings.h>
#endif

#ifdef MSDOS
# include <stdlib.h>
# include <errno.h>
# include <time.h>
# include <io.h>
# include <sys/types.h>
# include <sys/stat.h>
#endif

#include "nuread.h"
#include "nuadd.h"
#include "nupak.h"
#include "nuetc.h"

#ifdef DATAGENERAL					/* BAK */
# ifdef AOSVS						/* BAK */
#  define BROKEN_ON_MVs				/* MV/UX is NOT a full UNIX */
# endif						/* implem. so we just skip */
#endif						/* the 'UNIX' code on MVs */

#define MAXGSPREFIX 64

static BOOLEAN domove;    /* are we M)oving the files in? */
static BOOLEAN docreate;  /* using the 'C' option? */


/*
 * Expand command args into filenames
 * Stuff number of names into int; build File Information Array.
 * (this routine is heavily implementation-specific, since no two systems
 *  expand wildcards or deal with subdirectories in the same way).
 *
 * Recursively expands subdirectories, unless doSubdir is FALSE.
 */
int
EvalArgs(count, names, FIArray, first)
int count;	/* #of filenames */
char **names;	/* array of file names */
file_info *FIArray[];  /* array to fill with file info */
BOOLEAN first;	/* first time through? */
{
    static char *procName = "EvalArgs";
#ifdef UNIX
    /* UNIX shells (sh, csh) won't expand subdirectories, but they do
     * expand wildcards in arguments before we get them
     */
    static int idx;
    struct stat st;
    char *cp;  /* temp char pointer */
    /* dir stuff */
    int newcount;
    char **newnames;
#ifndef BROKEN_ON_MVs
    DIR *dirp;
#endif
#ifdef SYSV
# ifdef XENIX386
    struct direct *dp;		/* XENIX is like SYSV but with BSD dir stuff */
# else
    struct dirent *dp;
# endif
#else
    struct direct *dp;
#endif /*SYSV*/
    int nmlen;

    if (first) idx = 0;

    while (count--) {
	FIArray[idx] = (file_info *) Malloc(sizeof(file_info));

	if (stat(*names, &st) < 0) {  /* get file info */
	    if (errno == ENOENT) {
		fprintf(stderr, "%s: '%s' not found\n", prgName, *names);
		names++;
		continue;  /* with while */
	    }
	    Fatal("Bad stat()", procName);
	}

	if ((st.st_mode & S_IFDIR) && doSubdir) {  /* is it a directory? */
# ifndef BROKEN_ON_MVs
	    newnames = (char **) Malloc(MAXARGS * sizeof(char *));
	    strcpy(tmpNameBuf, *names);  /* earlier dir stuff */
	    strcat(tmpNameBuf, "/");
	    nmlen = strlen(tmpNameBuf);

	    if ((dirp = opendir(*names)) == NULL)
		Fatal("Unable to open subdirectory", procName);
	    for (newcount=0, dp=readdir(dirp); dp != NULL; dp=readdir(dirp)) {
		if ((!strcmp(dp->d_name, ".")) || (!strcmp(dp->d_name, "..")))
		    continue;  /* with for */
#ifdef SYSV
		newnames[newcount] = (char *)Malloc(nmlen+strlen(dp->d_name)+1);
#else
		newnames[newcount] = (char *)Malloc(nmlen + dp->d_namlen +1);
#endif
		strcpy(newnames[newcount], tmpNameBuf);
		strcat(newnames[newcount], dp->d_name);  /* append the name */
		newcount++;
	    }
	    closedir(dirp);

	    EvalArgs(newcount, newnames, FIArray, FALSE);  /* do subdir */

	    while (newcount-- > 0)  /* free up the space we allocated */
		free(newnames[newcount]);
	    free(newnames);

	    names++;
# else								/* BAK */
	    printf("Help, I ran into a directory and can't handle it!\n");
# endif								/* BAK */
	} else if ((st.st_mode & S_IFDIR) && !doSubdir) {
	    /* maybe print message? */
	    names++;
	    continue;	/* with while */
	} else if (st.st_mode & S_IFREG) {
	    FIArray[idx]->eof = (long) st.st_size;

	    if (st.st_mode & S_IWRITE)  /* write permission enabled? */
		FIArray[idx]->fAccess = (fourbyt) 0x00e3;	/* locked */
	    else
		FIArray[idx]->fAccess = (fourbyt) 0x0021;	/* unlocked */

	    FIArray[idx]->fileType = defFileType;
	    FIArray[idx]->auxType = diskData ? st.st_size/512 : defAuxType;
	    FIArray[idx]->fileSysID = 0x0001;  /* ProDOS */
	    FIArray[idx]->fileSysInfo = 0x2f;  /* '/' */
	    ExpandTime(&st.st_mtime, &FIArray[idx]->create_dt);  /*use mod.. */
	    ExpandTime(&st.st_mtime, &FIArray[idx]->mod_dt); /*time for both */
	    FIArray[idx]->marked = FALSE;

	    FIArray[idx]->pathname = (char *) Malloc(strlen(*names)+1);
	    strcpy(FIArray[idx]->pathname, *names);
	    FIArray[idx]->store_name = (char *) Malloc(strlen(*names)+1);
	    cp = *names;
	    while (*cp == '/') cp++;  /* advance past leading '/' */
	    strcpy(FIArray[idx]->store_name, cp);  /* can't otherwise fix */

	    names++;
	    idx++;
	} else {
	    printf("Unknown storage type for '%s'\n", *names);
	    names++;
	    continue;	/* with while */
	}

    }
    return (idx);

#else /* UNIX */
# ifdef APW
    static int idx;  /* will eventually hold the total #of filenames */
    char *nextname = (char *) Malloc(MAXFILENAME);  /* for subdir expand */
    char prefix[MAXGSPREFIX+1];  /* Max ProDOS prefix size; now 64 */
    char *fnptr;
    FileRec finfo_p;
    PrefixRec prefix_p;
    OpenRec open_p;
    EOFRec eof_p;

    if (first) idx = 0;

    prefix_p.prefixNum = 0;  /* current dir */
    prefix_p.prefix = prefix;  /* prefix buffer */
    GET_PREFIX( &prefix_p );
    ToolErrChk();
    p2cstr(prefix);

    while (count) {
	strcpy(tmpNameBuf, *names);
	c2pstr(tmpNameBuf);
	INIT_WILDCARD(tmpNameBuf, 0);
	ToolErrChk();

	while (*NEXT_WILDCARD(tmpNameBuf)) {
	    if (idx >= MAXARGS) {
		fprintf(stderr, "Too many files (%d, %d max)\n", idx, MAXARGS);
		Quit (-1);
	    }

	    finfo_p.pathname = tmpNameBuf;
	    GET_FILE_INFO( &finfo_p );
	    ToolErrChk();

	    open_p.openPathname = tmpNameBuf;
	    OPEN( &open_p );
	    ToolErrChk();

	    eof_p.eofRefNum = open_p.openRefNum;
	    GET_EOF( &eof_p );
	    ToolErrChk();

	    CLOSE( &open_p );
	    ToolErrChk();

	    p2cstr(tmpNameBuf);  /* also does p2cstr(finfo_p.pathname) */
	    switch (finfo_p.storageType) {
	    case 0x00:	/* standard ProDOS storage types */
	    case 0x01:
	    case 0x02:
	    case 0x03:
		FIArray[idx] = (file_info *) Malloc(sizeof(file_info));

		FIArray[idx]->eof = eof_p.eofPosition;
		FIArray[idx]->fAccess = finfo_p.fAccess;
		FIArray[idx]->fileType = (fourbyt) finfo_p.fileType;
		FIArray[idx]->auxType = (fourbyt) finfo_p.auxType;
		FIArray[idx]->storageType = finfo_p.storageType;
		FIArray[idx]->fileSysID = 0x0001;  /* ProDOS */
		FIArray[idx]->fileSysInfo = 0x2f;  /* '/' */
		ExpandTime(&finfo_p.createDate, &FIArray[idx]->create_dt);
		ExpandTime(&finfo_p.modDate, &FIArray[idx]->mod_dt);
		FIArray[idx]->marked = FALSE;

		FIArray[idx]->pathname = (char *) Malloc(strlen(tmpNameBuf)+1);
		strcpy(FIArray[idx]->pathname, tmpNameBuf);

		/* are we adding from current directory? */
		if (!strncmp(tmpNameBuf, prefix, strlen(prefix))) {
		    FIArray[idx]->store_name =			      /* yes */
		       (char *) Malloc(strlen(tmpNameBuf) - strlen(prefix) +1);
		  strcpy(FIArray[idx]->store_name, tmpNameBuf+ strlen(prefix));
		} else {
		    fnptr = RINDEX(tmpNameBuf, '/') + 1;	      /* no  */
		    FIArray[idx]->store_name = (char *)Malloc(strlen(fnptr)+1);
		    strcpy(FIArray[idx]->store_name, fnptr);
		}
		idx++;
		break;

	    case 0x05:
		printf("Can't handle Extended file '%s'\n", tmpNameBuf);
		break;
	    case 0x0d:
		if (doSubdir) {
		    strcpy(nextname, tmpNameBuf);  /* make new copy */
		    strcat(nextname, "/=");  /* APW-only wildcard */
		    EvalArgs(1, &nextname, FIArray, FALSE);  /* read subdir */
		}
		break;
	    default:
		printf("Unknown storage type for '%s'\n", tmpNameBuf);
		break;
	    }

	} /* inner while */

	names++, count--;
    }  /* outer while */
    free (nextname);
    return (idx);
# endif /* APW */

# ifdef MSDOS
    /* MS-DOS or other shell wildcard expansion here */
    int idx, error;
    struct stat fStat;

    idx = 0;

    while (count--) {

	error = stat (*names, &fStat);

	/* If the filename is a directory, we need to expand that too! */

	if (!error) {
	    FIArray[idx]              = (file_info *) Malloc(sizeof(file_info));
	    FIArray[idx]->pathname    = (char *) Malloc(strlen(*names)+1);
	    strcpy(FIArray[idx]->pathname, *names);
	    FIArray[idx]->store_name  = (char *) Malloc(strlen(*names)+1);
	    strcpy(FIArray[idx]->store_name, *names);
	    FIArray[idx]->fAccess     = 0x00e3L;  /* unlocked */
	    FIArray[idx]->fileType    = defFileType;
	    FIArray[idx]->auxType     = diskData ? fStat.st_size/512:defAuxType;
	    FIArray[idx]->storageType = 0x0000;
	    FIArray[idx]->fileSysID   = 0x0001;  /* ProDOS */
	    FIArray[idx]->fileSysInfo = 0x1c;  /* '\' */
	    ExpandTime(&fStat.st_ctime, &FIArray[idx]->create_dt);
	    ExpandTime(&fStat.st_mtime, &FIArray[idx]->mod_dt);
	    FIArray[idx]->eof = fStat.st_size;

	    FIArray[idx]->marked = FALSE;
	    idx++;
	}
	names++;
    }
    return (idx);
# endif /* MDOS */

# ifndef APW
# ifndef MSDOS
    /* nothing else defined */

    /* +PORT+ */
    printf("\n[other] wildcard expansion/file info needs work\n");
    while (count--) {
	FIArray[count] = (file_info *) Malloc(sizeof(file_info));

	FIArray[count]->pathname = (char *) Malloc(strlen(*names)+1);
	strcpy(FIArray[count]->pathname, *names);
	FIArray[count]->store_name = (char *) Malloc(strlen(*names)+1);
	strcpy(FIArray[count]->store_name, *names);
	FIArray[count]->fAccess = 0x00e3L;  /* unlocked */
	FIArray[count]->fileType = 0x0006L;  /* BIN */
	FIArray[count]->auxType = 0L;
	FIArray[count]->storageType = 0x0000;
	FIArray[count]->fileSysID = 0x0001;  /* ProDOS */
	FIArray[count]->fileSysInfo = 0x1c;  /* '\' */
	ExpandTime((char *) NULL, &FIArray[count]->create_dt);
	ExpandTime((char *) NULL, &FIArray[count]->mod_dt);
	FIArray[count]->marked = FALSE;

	names++;
    }
    return (count);
# endif /* none2 */
# endif /* none1 */
#endif /* UNIX */
}


/*
 * Add a file onto the end of an archive; does not check to see if an entry
 * already exists.
 *
 * This creates the record entry, and calls subroutines to add the various
 * threads.  The archive fd should be open, the file fd should not.  Returns
 * the size of the record added.
 */
long
AddFile(arcfd, infoptr)
int arcfd;
file_info *infoptr;
{
    int srcfd;	/* file to add */
    onebyt *recBuf;  /* record header block */
    twobyt *twoptr;
    THblock thread[1];	/* thread block */
    twobyt CRC;
    int idx;
    fourbyt total_threads;
    long recposn;  /* file posn for record entry */
    long thposn;   /* file posn for last thread */
    long tmpposn;  /* temporary file posn */
    static char *procName = "AddFile";

    if (verbose) {
	printf("Adding '%s' (data)...",
					  infoptr->store_name, infoptr->eof);
	fflush(stdout);
    }

    recBuf = (onebyt *) Malloc(ATTSIZE);
    for (idx = 0; idx < ATTSIZE; idx++)  /* zero the buffer */
	*(recBuf+idx) = 0;

    total_threads = 0;

    strncpy((char *) recBuf+0, (char *) RecordID, 4);
    twoptr = (twobyt *) (recBuf+6);
    *twoptr = ATTSIZE;	/* don't have an attrib_count... */
    HiSwap((onebyt *) recBuf, 6, 7);
    twoptr = (twobyt *) (recBuf+8);
    *twoptr = OURVERS;	/* store new record with our rec vers */
    HiSwap((onebyt *) recBuf, 8, 9);
    /* total_threads */
/*    BCopy((onebyt *) &total_threads, (onebyt *) recBuf+10, 2, TRUE); */
    *(recBuf+12) = 0;  /* reserved1 */
    *(recBuf+13) = 0;
    BCopy((onebyt *) &infoptr->fileSysID, (onebyt *) recBuf+14, 2, TRUE);
    BCopy((onebyt *) &infoptr->fileSysInfo, (onebyt *) recBuf+16, 1, TRUE);
    *(recBuf+17) = 0;  /* reserved2 */
    BCopy((onebyt *) &infoptr->fAccess, (onebyt *) recBuf+18, 4, TRUE);
    BCopy((onebyt *) &infoptr->fileType, (onebyt *) recBuf+22, 4, TRUE);
    BCopy((onebyt *) &infoptr->auxType, (onebyt *) recBuf+26, 4, TRUE);
    BCopy((onebyt *) &infoptr->create_dt, (onebyt *) recBuf+32,
	sizeof(Time), FALSE);
    BCopy((onebyt *) &infoptr->mod_dt, (onebyt *) recBuf+40, sizeof(Time),
	FALSE);
    BCopy((onebyt *) GetTime(), (onebyt *) recBuf+48, sizeof(Time), FALSE);
    twoptr = (twobyt *) (recBuf + (ATTSIZE - 2));
    *twoptr = strlen(infoptr->store_name);

    /* correct strlen ordering */
    HiSwap((onebyt *) recBuf, ATTSIZE-2, ATTSIZE-1);

    thread[0].thread_class = 0x0002;  /* data */
    HiSwap((onebyt *) &thread[0].thread_class, 0, 1);
    thread[0].thread_kind = diskData ? 0x0001 : 0x0000; /* disk or data fork */
    HiSwap((onebyt *) &thread[0].thread_kind, 0, 1);
    thread[0].thread_format = 0x0000;  /* filled in later */
    thread[0].thread_crc = 0x0000;  /* not supported yet */
    /* so I don't forget if I support these */
    HiSwap((onebyt *) &thread[0].thread_crc, 0, 1); 
    thread[0].thread_eof = infoptr->eof;
    HiSwap((onebyt *) &thread[0].thread_eof, 0, 3);
    HiSwap((onebyt *) &thread[0].thread_eof, 1, 2);
    thread[0].comp_thread_eof = -1L;  /* filled in later */
    total_threads++;

    BCopy((onebyt *) &total_threads, (onebyt *) recBuf+10, 4, TRUE);

    /*
     * Because we don't know CRCs or compressed size yet, we must:
     * skip record entry and filename.
     * for each thread:
     *	 skip thread entry, write data, move back, write thread entry.
     * move back, write record entry and filename.
     * move forward to next position.
     */
    if ((srcfd = open(infoptr->pathname, O_RDONLY | O_BINARY)) < 0)
	Fatal("Unable to open file", procName);

    recposn = lseek(arcfd, (off_t) 0, S_REL);	/* save record posn */
    if (lseek(arcfd, (off_t) (ATTSIZE + strlen(infoptr->store_name)), S_REL)<0)
	Fatal("Bad seek (R.rel)", procName);

    /* loop... */
    thposn = lseek(arcfd, (off_t) 0, S_REL);  /* save thread posn */
    if (lseek(arcfd, (off_t) THsize, S_REL) < 0)
	Fatal("Bad seek (Th)", procName);

    /*
     * since we can store files as being packed without actually packing them,
     * we need to check "dopack" to see if we want packMethod or zero.  Note
     * that packing can fail for various reasons; the value returned by
     * PackFile() is the actual algorithm used to pack the file.
     *
     * NuLib uses version 0 records; thread_crcs are not stored.
     */

    thread[0].thread_format = 
	    PackFile(srcfd,arcfd, infoptr->eof, dopack ? packMethod:0, pakbuf);
    if (!dopack) thread[0].thread_format = packMethod;  /* for S subopt */
    HiSwap((onebyt *) &thread[0].thread_format, 0, 1);
    thread[0].comp_thread_eof = (fourbyt) packedSize;
    HiSwap((onebyt *) &thread[0].comp_thread_eof, 0, 3);  /* correct ordering*/
    HiSwap((onebyt *) &thread[0].comp_thread_eof, 1, 2);
    tmpposn = lseek(arcfd, (off_t) 0, S_REL);

    if (lseek(arcfd, (off_t) thposn, S_ABS) < 0)  /* seek back to thread posn */
	Fatal("Bad seek (Th2)", procName);
    if (write(arcfd, &thread[0], THsize) < THsize)  /* write updated thread */
	Fatal("Unable to write thread", procName);
    if (lseek(arcfd, (off_t)tmpposn, S_ABS) < 0)  /*seek back to where we were*/
	Fatal("Bad seek (TmpA)", procName);
    /* ...loop end */

    if (close(srcfd) < 0)
	Fatal("Unable to close file", procName);

    CRC = CalcCRC(0, (onebyt *) recBuf+6, ATTSIZE-6);
    CRC = CalcCRC(CRC, (onebyt *) infoptr->store_name,
	strlen(infoptr->store_name));
    CRC = CalcCRC(CRC, (onebyt *) &thread[0], THsize);
    twoptr = (twobyt *) (recBuf+4);
    *twoptr = CRC;
    HiSwap((onebyt *) recBuf, 4, 5);

    tmpposn = lseek(arcfd, (off_t) 0, S_REL);	   /* record posn (next posn) */
    if (lseek(arcfd, (off_t)recposn, S_ABS) < 0) /* seek back to record entry */
	Fatal("Bad seek (R.abs)", procName);
    if (write(arcfd, recBuf, ATTSIZE) < ATTSIZE)
	Fatal("Unable to write record", procName);
    if (write(arcfd, infoptr->store_name, strlen(infoptr->store_name))
				     < strlen(infoptr->store_name))
	Fatal("Unable to store filename", procName);
    if (lseek(arcfd, (off_t)tmpposn, S_ABS) < 0) /*seek back to where we were */
	Fatal("Bad seek (TmpB)", procName);

    if (verbose) printf("done.\n");
    free(recBuf);

    /* switch ordering back */
    HiSwap((onebyt *) &thread[0].comp_thread_eof, 0, 3);
    HiSwap((onebyt *) &thread[0].comp_thread_eof, 1, 2);
    return ( (long) (THsize * total_threads) + (long) ATTSIZE +
	  (long) strlen(infoptr->store_name) + thread[0].comp_thread_eof);
}


/*
 * Certain options can cause an archive to be created (add, create, move).
 * If the archive does not already exist, then an empty file is created (of
 * type $e0/8002 under ProDOS) and an empty archive struct is built.
 *
 * Note that this requires certain options to deal with archive structures that
 * do not have any records, and archive files that are empty.
 *
 * If the file exists, this will call NuRead() to read it; otherwise, it will
 * create it.
 */
static ListHdr *
CreateMaybe(filename)
char *filename;
{
    ListHdr *archive;
    MHblock *MHptr;
    onebyt *bufPtr;
    twobyt *twoptr;
    fourbyt *fourptr;
    int idx;
#ifdef APW
    FileRec create_p;
#endif
    static char *procName = "CreateMaybe";

    if (Exists(filename)) {
	archive = NuRead(filename);
	return (archive);
    }

    if (!docreate)
	printf("Archive does not exist; creating archive file...\n");

    archive = (ListHdr *) Malloc(sizeof(ListHdr));
    archive->arc_name = (char *) Malloc(strlen(filename)+1);
    strcpy(archive->arc_name, filename);
    archive->MHptr = (MHblock *) Malloc(sizeof(MHblock));
    archive->RNodePtr = (RNode *) NULL;
    archive->nextposn = (long) MHsize;

    bufPtr = (onebyt *) archive->MHptr;
    for (idx = 0; idx < MHsize; idx++)
	*(bufPtr+idx) = '\0';

    /* total_records -> zero */
    MHptr = archive->MHptr;
    strncpy((char *) MHptr->ID, (char *) MasterID, 6);
    BCopy((onebyt *) GetTime(), (onebyt *) &(MHptr->arc_create_when),8, FALSE);
    BCopy((onebyt *) bufPtr+12, (onebyt *) &(MHptr->arc_mod_when), 8, FALSE);
    fourptr = (fourbyt *) (&(MHptr->master_eof));
    *fourptr = (fourbyt) MHsize;

/*    twoptr = (twobyt *) (&(MHptr->master_crc));
    *twoptr = CalcCRC(0, (onebyt *) bufPtr+8, MHsize-8); */

    ArcfiCreate(filename);  /* create SHK file */
    return (archive);
}


/*
 * Return a pointer to a valid Master Header block
 * Anything that isn't set to a default value needs to be passed as a
 * parameter [ right now I can't remember why ].
 */
onebyt *
MakeMHblock(archive, total_records, master_eof)
ListHdr *archive;
fourbyt total_records;
fourbyt master_eof;
{
    static onebyt buf[MHsize];  /* must be static */
    twobyt *twoptr;
    fourbyt *fourptr;
    int idx;
    static char *procName = "MakeMHblock";

    for (idx = 0; idx < MHsize ; idx++)
	buf[idx] = '\0';

    /* messy... should've used MHptr->thing here, but if it ain't broke... */
    strncpy((char *) buf, (char *) MasterID, 6);
    BCopy((onebyt *) &total_records, (onebyt *) &buf[8], 4, TRUE);
    BCopy((onebyt *) &archive->MHptr->arc_create_when, (onebyt *) &buf[12],
	sizeof(Time), FALSE);
    BCopy((onebyt *) GetTime(), (onebyt *) &buf[20], sizeof(Time), FALSE);
    twoptr = (twobyt *) &buf[28];  /* master version */
    *twoptr = OURMVERS;
    HiSwap((onebyt *) buf, 28, 29);  /* correct byte ordering */
    BCopy((onebyt *) &master_eof, (onebyt *) &buf[38], 4, TRUE);
    twoptr = (twobyt *) &buf[6];
    *twoptr = CalcCRC(0, (onebyt *) &buf[8], MHsize-8);
    HiSwap((onebyt *) buf, 6, 7);

    return (buf);
}


/*
 * Add files to archive
 *
 * Read files from disk, adding them to the end of the archive as we go.
 * Update the master header block after all files have been added.
 */
static void
Add(filename, namecount, names)
char *filename;
int namecount;
char **names;
{
    ListHdr *archive;
    int arcfd;
    file_info *FIArray[MAXARGS];  /* entries malloc()ed by EvalArgs */
    int idx;
    onebyt *mptr;  /* points to a MHblock suitable for writing */
    long addSize;
    static char *procName = "Add";

    /* expand wildcards/subdirectories, and get file info */
    namecount = EvalArgs(namecount, names, FIArray, TRUE);
    if (!namecount) {
	if (verbose) printf("No files selected.\n");
	Quit (0);
    }

    archive = CreateMaybe(filename);
    if ((arcfd = open(archive->arc_name, O_RDWR | O_BINARY)) < 0)
	Fatal("Unable to open archive", procName);
    if (lseek(arcfd, (off_t)archive->nextposn, S_ABS) < 0)  /* seek to end */
	Fatal("Unable to seek in archive", procName);

    for (idx = 0 ; idx < namecount; idx++) {
#ifdef APW
	if (STOP()) Quit(1);  /* check for OA-. */
#endif
	addSize = AddFile(arcfd, FIArray[idx]);
	archive->MHptr->master_eof += addSize;
	archive->nextposn += addSize;
	archive->MHptr->total_records++;
    }

    mptr = MakeMHblock(archive, archive->MHptr->total_records,
		archive->MHptr->master_eof);
    if (lseek(arcfd, (off_t) 0, S_ABS) < 0)
	Fatal("Unable to rewind archive for master header", procName);

    if (write(arcfd, mptr, MHsize) < MHsize)
	Fatal("Unable to update master header", procName);
    if (close(arcfd) < 0)
	Fatal("Unable to close archive", procName);

    if (domove) {
	if (verbose) printf("Deleteing files...\n");
	for (idx = 0; idx < namecount; idx++) {
	    if (verbose) {
		printf("%s...", FIArray[idx]->pathname);
		fflush(stdout);
	    }
	    if (unlink(FIArray[idx]->pathname) < 0) {
		if (verbose) printf("failed.\n");
	    } else {
		if (verbose) printf("done.\n");
	    }
	}
    }
}


/*
 * Main entry point for adding files.
 */
void
NuAdd(filename, namecount, names, options)
char *filename;
int namecount;
char **names;
char *options;
{
    char *optr;
    int idx;
    char type[5];
    static char *procName = "NuAdd";

    if (*options == 'm')
	domove = TRUE;

    if (*options == 'c')
	docreate = TRUE;

    /* change T subopt to convert FROM current system TO <subopt> */
    if (transfrom >= 0) {
	transto = transfrom;
	transfrom = -1;
    }

    Add(filename, namecount, names);  /* do main processing */
}

