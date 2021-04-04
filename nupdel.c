/*
 * nudel.c/nuupd.c - operations which delete/update/freshen a NuFX archive
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
#ifdef BSD43
# include <strings.h>
#else  /* SYSV, APW, MSC */
# include <string.h>
#endif

#ifdef UNIX
# include <errno.h>
# include <sys/types.h>
#endif
#ifdef APW
# include <types.h>
# include <prodos.h> /* ? */
# include <shell.h>
# include <strings.h>  /* APW string ops */
#endif
#ifdef MSDOS
# include <io.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <errno.h>
#endif

#include "nuread.h"
#include "nuadd.h"  /* AddFile(), etc. */
#include "nupak.h"  /* uses PAKBUFSIZ */
#include "nupdel.h"
#include "nuetc.h"

static BOOLEAN dofreshen;  /* do freshen instead of update? */

/* delete routines */

/*
 * Rebuild an archive, excluding files marked as deleted.
 * Does not use absolute position values; just seeks along.  The archive
 * should be seeked just beyond the master header block.
 */
static void
RebuildArchive(arcfd, archive, tmpname, remaining)
int arcfd;
ListHdr *archive;
char *tmpname;
long remaining;
{
    int dstfd;	/* destination filename */
    onebyt *mptr;
    RNode *RNodePtr;
    TNode *TNodePtr;
    int rec, thread;
    long size;
    long master_eof;
#ifdef APW
    FileRec create_p;
#endif
    static char *procName = "RebuildArchive";

    if (verbose) { printf("Building new archive file...");  fflush(stdout); }

    ArcfiCreate(tmpname);	/* create file */
    master_eof = (long) MHsize;

    if ((dstfd = open(tmpname, O_WRONLY | O_TRUNC | O_BINARY,(mode_t)WPERMS))<0)
	Fatal("Unable to open dest file", procName);
    if (lseek(dstfd, (off_t) MHsize, S_ABS) < 0)
	Fatal("Unable to lseek past dest mhblock", procName);

    RNodePtr = archive->RNodePtr;

    /* copy the surviving records to the destination file */
    for (rec = 0; rec < archive->MHptr->total_records; rec++) {
#ifdef APW
	if (STOP()) { printf("aborting.\n"); Quit(1); }
#endif
	size = (long) RNodePtr->RHptr->attrib_count;
	size += (long) RNodePtr->filename_length;
	TNodePtr = RNodePtr->TNodePtr;
	for (thread=0; thread < (int)RNodePtr->RHptr->total_threads; thread++){
	    if (TNodePtr == (TNode *) NULL) {
		fprintf(stderr, "Internal error: Bad thread structure\n");
		Quit(-1);
	    }
	    size += (long) THsize;
	    size += TNodePtr->THptr->comp_thread_eof;
	    TNodePtr = TNodePtr->TNext;
	}

	if (!RNodePtr->filename[0]) {
	    if (lseek(arcfd, (off_t) size, S_REL) < 0)
		Fatal("Unable to seek past deleted record", procName);
	} else {
	    FCopy(arcfd, dstfd, size, pakbuf, FALSE);
	    master_eof += size;
	}

	RNodePtr = RNodePtr->RNext;  /* move on to next record */
    }

    mptr = MakeMHblock(archive, remaining, master_eof);  /* build mheader */
    if (lseek(dstfd, (off_t) 0, S_ABS) < 0)
	Fatal("Unable to seek back in dest file", procName);
    if (write(dstfd, mptr, MHsize) < MHsize)
	Fatal("Unable to write master header", procName);

    if (close(dstfd) < 0)
	Fatal("Unable to close archive", procName);
    if (verbose) printf("done.\n");
}


/*
 * Delete files from archive
 *
 * Scan archive, deleting files which match the strings in "names".
 */
static void
Delete(filename, namecount, names)
char *filename;
int namecount;
char **names;
{
    ListHdr *archive;
    int arcfd;	/* archive file descriptor */
    int rec, idx;
    MHblock *MHptr;   /* Master Header block */
    RNode *RNodePtr;  /* Record Node */
    int len, *lentab;  /* hold strlen() of all names */
    char *pn;  /* archived pathname */
    long remaining;
    char *tmpname = (char *) Malloc(MAXFILENAME);
    static char *procName = "Delete";

    archive = NuRead(filename);
    if ((arcfd = open(archive->arc_name, O_RDONLY | O_BINARY)) < 0)
	Fatal("Unable to open archive", procName);

    lentab = (int *) Malloc( sizeof(int) * namecount ); /* calloc() is nicer */
    for (idx = 0; idx < namecount; idx++)  /* calc. once (for efficiency) */
	lentab[idx] = strlen(names[idx]);

    MHptr = archive->MHptr;
    RNodePtr = archive->RNodePtr;
    remaining = MHptr->total_records;

    /* main record read loop */
    for (rec = 0; rec < MHptr->total_records; rec++) {

	pn = RNodePtr->filename;
	len = strlen(pn);
	if (RNodePtr->RHptr->version_number > MAXVERS)
	    printf("WARNING: '%s' has unknown record version_number\n", pn);

	for (idx = 0; idx < namecount; idx++) {  /* find file in archive */
	    /* try to match argument with first few chars of stored filename */
	    /* or the entire filename, depending on EXPAND flag */
	    if ((len >= lentab[idx]) && doExpand ?
			(!strncasecmp(pn, names[idx], lentab[idx])) :
			(!strcasecmp(pn, names[idx])) ) {

		if (verbose) printf("Marking '%s' as deleted.\n", pn);
		RNodePtr->filename[0] = '\0';  /* mark as deleted */
		remaining--;
		break;	/* out of filename matching for-loop */
	    }
	}

	RNodePtr = RNodePtr->RNext;  /* move on to next record */
    }

    if (remaining == MHptr->total_records) {
	if (verbose) printf("No files selected; archive not modified\n");
	if (close(arcfd) < 0)
	    Fatal("Source (archive) close failed", procName);
	Quit (0);
    }
    if (remaining == 0L) {
	printf("All files in archive marked for deletion...");	fflush(stdout);
	if (close(arcfd) < 0)
	    Fatal("Source (archive) close failed", procName);
#ifdef APW
	if (STOP()) { printf("aborting.\n"); Quit (1); }
#endif
	printf(" deleteing archive file.\n");

	if (unlink(archive->arc_name) < 0)
	    Fatal("Unable to delete archive", procName);
    } else {
	tmpname = MakeTemp(tmpname);
#ifdef APW
	if (STOP()) { printf("aborting.\n"); Quit (1); }
#endif
	if (lseek(arcfd, (off_t) MHsize, S_ABS) < 0)
	    Fatal("Unable to seek past master block", procName);
	RebuildArchive(arcfd, archive, tmpname, remaining);

	if (close(arcfd) < 0)
	    Fatal("Source (archive) close failed", procName);
	if (verbose) {
	    printf("Deleteing old archive file...");
	    fflush(stdout);
	}
	if (unlink(archive->arc_name) < 0)
	    Fatal("Unable to delete original archive", procName);
	Rename(tmpname, archive->arc_name);
	if (verbose) printf("done.\n");

    }
    free (tmpname);
    free (lentab);
}


/*
 * Entry point for deleteing files from archive.
 */
void
NuDelete(filename, namecount, names, options)
char *filename;
int namecount;
char **names;
char *options;
{
    static char *procName = "NuDelete";

    /* presently does nothing */

    Delete(filename, namecount, names);
}


/**********  update routines  **********/

/*
 * Updates the archive.
 *
 * Evaluate the command line arguments and compare them with the files in
 * the archive.  Put the most recent copy of the file in a new archive file.
 * Essentially a combination of add and delete.
 *
 * This procedure is huge.  Someday I may clean this up a bit...
 */
static void
Update(archive, namecount, names)
ListHdr *archive;
int namecount;
char **names;
{
    int arcfd, dstfd;  /* archive file descriptor */
    static file_info *FIArray[MAXARGS];  /* entries malloc()ed by EvalArgs */
    unsigned int rec;
    int idx, thread;
    MHblock *MHptr;   /* Master Header block */
    RNode *RNodePtr;  /* Record Node */
    TNode *TNodePtr;   /* Thread block */
    char *pn;  /* archived pathname */
    BOOLEAN keeparc, gotone;
    char *tmpname = (char *) Malloc(MAXFILENAME);
    Time *atptr, *ftptr;
    long a_dt, f_dt;
    long size;
    fourbyt totalrecs, master_eof;
    onebyt *mptr;
    twobyt *twoptr;
    static char *procName = "Update";

    if ((arcfd = open(archive->arc_name, O_RDONLY | O_BINARY)) < 0)
	Fatal("Unable to open archive", procName);

    /* expand wildcards/subdirectories, and get info */
    namecount = EvalArgs(namecount, names, FIArray, TRUE);
    if (!namecount) {
	if (verbose) printf("No files selected; archive not modified.\n");
	Quit (0);
    }

    /*
     * For each file in the archive, check for an *exact* match with the
     * store_names in FIArray (case independent).  If a match is found,
     * compare the dates, and copy/add the most recent file.  If no match
     * is found, copy the file.  After all archived files have been processed,
     * add all remaining files specified on the command line (unless the
     * F)reshen option is used, in which case this exits).
     */

    MHptr = archive->MHptr;
    RNodePtr = archive->RNodePtr;
    gotone = FALSE;

    /* mark files that will be replaced */
    for (rec = 0; rec < MHptr->total_records; rec++) {
#ifdef APW
	if (STOP()) { printf("aborting.\n"); Quit (1); }
#endif
	pn = RNodePtr->filename;
	if (RNodePtr->RHptr->version_number > MAXVERS)
	    printf("WARNING: '%s' has unknown record version_number\n", pn);

	for (idx = 0; idx < namecount; idx++) {  /* find file in archive */
	    /* try to match argument with first few chars of stored filename */
	    if (!strcasecmp(pn, FIArray[idx]->store_name)) {
		atptr = &RNodePtr->RHptr->mod_when;
		ftptr = &FIArray[idx]->mod_dt;

		/* compare month/year [ I think it's best-case faster... ] */
		a_dt = (atptr->year * 12) + atptr->month;
		f_dt = (ftptr->year * 12) + ftptr->month;
		if (a_dt > f_dt)	/* archive is more recent? */
		    keeparc = TRUE;
		else if (a_dt < f_dt)	/* file is more recent? */
		    keeparc = FALSE;
		else {	/* year & month match, check rest */
		    a_dt = (atptr->day * 86400L) + (atptr->hour * 3600) +
			   (atptr->minute * 60) + (atptr->second);
		    f_dt = (ftptr->day * 86400L) + (ftptr->hour * 3600) +
			   (ftptr->minute * 60) + (ftptr->second);
		    if (a_dt < f_dt)
			keeparc = FALSE;
		    else  /* (a_dt >= f_dt) */
			keeparc = TRUE;
		}

		if (!keeparc) {  /* not keeping; mark as being replaced */
#ifndef APW  /* APW uses actual filetype; other systems keep old */
		    FIArray[idx]->fileType = RNodePtr->RHptr->file_type;
		    FIArray[idx]->auxType = RNodePtr->RHptr->extra_type;
#endif
		    RNodePtr->RHptr->version_number = 65535; /*can't do fname*/
		    twoptr = (twobyt *) RNodePtr->filename;
		    *twoptr = idx;  /* null filename -> problems */
		    gotone = TRUE;
		}
		FIArray[idx]->marked = TRUE;  /* MARK as processed */
	    }
	}

	RNodePtr = RNodePtr->RNext;  /* move on to next record */
    }

    totalrecs = MHptr->total_records;  /* none will be deleted */
    if (!dofreshen) {  /* add new files? */
	for (idx = 0; idx < namecount; idx++) {  /* handle unmatched args */
	    if (!FIArray[idx]->marked) {
		gotone = TRUE;
		totalrecs++;  /* count new ones too */
	    }
	}
    }
    if (!gotone) {
	if (verbose) printf("No files need updating; archive not modified\n");
	if (close(arcfd) < 0)
	    Fatal("Source (archive) close failed", procName);
	Quit (0);
    }

    /*
     * Rebuild archive file
     */
    if (verbose) printf("Building new archive file...\n");
    tmpname = MakeTemp(tmpname);
    ArcfiCreate(tmpname);

    master_eof = (long) MHsize;

    if (lseek(arcfd, (off_t) MHsize, S_ABS) < 0)
	Fatal("Bad archive seek", procName);

    if ((dstfd = open(tmpname, O_RDWR | O_TRUNC | O_BINARY)) < 0)
	Fatal("Unable to open dest file", procName);
    if (lseek(dstfd, (off_t) MHsize, S_ABS) < 0)
	Fatal("Bad dest seek", procName);	/* leave space for later */

    RNodePtr = archive->RNodePtr;
    for (rec = 0; rec < MHptr->total_records; rec++) {
	size = (long) RNodePtr->RHptr->attrib_count;
	size += (long) RNodePtr->filename_length;
	TNodePtr = RNodePtr->TNodePtr;
	for (thread=0; thread < (int)RNodePtr->RHptr->total_threads; thread++){
	    if (TNodePtr == (TNode *) NULL) {
		fprintf(stderr, "Internal error: Bad thread structure\n");
		Quit (-1);
	    }
	    size += (long) THsize;
	    size += TNodePtr->THptr->comp_thread_eof;
	    TNodePtr = TNodePtr->TNext;
	}
	/* we now know the size; either copy the old or replace with new */
	if (RNodePtr->RHptr->version_number != 65535) { /* file not replaced */
/*	    if (verbose) {
 *		printf("Copying '%s'...", RNodePtr->filename);
 *		fflush(stdout);
 *	    }
 */
	    FCopy(arcfd, dstfd, size, pakbuf, FALSE);
	    master_eof += (fourbyt) size;
/*	    if (verbose) printf("done.\n");
 */
	} else {  /* file replaced; skip orig and add new */
	    if (lseek(arcfd, (off_t) size, S_REL) < 0)
		Fatal("Bad skip seek", procName);
	    twoptr = (twobyt *) RNodePtr->filename;
	    idx = *twoptr;
	    if (verbose) printf("Replacing/");  /* +"Adding 'filename'..." */
	    master_eof += AddFile(dstfd, FIArray[idx]);
	}

	RNodePtr = RNodePtr->RNext;  /* move on to next record */
    }

    if (!dofreshen) {
	for (idx = 0 ; idx < namecount; idx++) {
#ifdef APW
	    if (STOP()) Quit(1);  /* check for OA-. */
#endif
	    if (!FIArray[idx]->marked) {
		master_eof += AddFile(dstfd, FIArray[idx]);
	    }
	}
    }

    /* build master header */
    mptr = MakeMHblock(archive, totalrecs, master_eof);
    if (lseek(dstfd, (off_t) 0, S_ABS) < 0)
	Fatal("Bad lseek for master header", procName);
    if (write(dstfd, mptr, MHsize) < MHsize)
	Fatal("Unable to write master header", procName);

    if (close(arcfd) < 0)
	Fatal("Source (old archive) close failed", procName);
    if (close(dstfd) < 0)
	Fatal("Destination (new archive) close failed", procName);

    if (verbose) { printf("Deleteing old archive file...");  fflush(stdout); }
    if (unlink(archive->arc_name) < 0)
	Fatal("Unable to delete original archive", procName);
    Rename(tmpname, archive->arc_name);
    if (verbose) printf("done.\n");

    free (tmpname);
}


/*
 * Update files in archive
 *
 * This part just evaluates the options, sets parms, and calls Update().
 */
void
NuUpdate(filename, namecount, names, options)
char *filename;
int namecount;
char **names;
char *options;
{
    ListHdr *archive;
    static char *procName = "NuUpdate";

    if (*options == 'f') dofreshen = TRUE;
    else dofreshen = FALSE;

    /* change T subopt to convert FROM current system TO <subopt> */
    if (transfrom >= 0) {
	transto = transfrom;
	transfrom = -1;
    }

    archive = NuRead(filename);
    Update(archive, namecount, names);
}

