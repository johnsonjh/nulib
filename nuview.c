/*
 * nuview.c - prints the contents of a NuFX archive
 *
 * NuLib v3.2  March 1992  Freeware (distribute, don't sell)
 * By Andy McFadden (fadden@uts.amdahl.com)
 */
#ifdef APW
segment "NuMain"
#endif

#include "nudefs.h"
#include "stdio.h"
#ifdef BSD43
# include <strings.h>
#else  /* SYSV, APW, MSC */
# include <string.h>
#endif

#ifdef APW
# include <shell.h>
#endif

#include "nuview.h"
#include "nuread.h"
#include "nuetc.h"


/*
 * String definitions for NuView
 */
/* unknown value msg */
char *unknownStr = "[ unknown ]";

/* weekDay values */
char *WD[8] = { "[ null ]", "Sunday", "Monday", "Tuesday", "Wednesday",
		"Thursday", "Friday", "Saturday" };

/* month values */
char *MO[13] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
		"Aug", "Sep", "Oct", "Nov", "Dec" };

/* thread_class */
/*#define TCn 4*/
char *TC[TCn] = { "Message_thread", "Control_thread", "Data_thread",
		"Filename_thread" };

/*#define TKn 3  /* max #of thread_kinds in a thread_class */
char *TK[TCn][TKn] = {
		{ "ASCII text", "ASCII text (predef size)", "<undef>" },
		{ "Create directory", "<undef>", "<undef>" },
		{ "File data_fork", "Disk image", "File resource_fork" },
		{ "Generic filename", "<undef>", "<undef>" } };

/* thread_format */
/*#define TFn 6*/
char *TF[TFn] = { "Uncompressed", "SQueezed (SQ/USQ)",
		"Dynamic LZW Type I (ShrinkIt)",
		"Dynamic LZW Type II (ShrinkIt)", "12-bit UNIX compress",
		"16-bit UNIX compress" };

/* brief thread format */
/*#define BTFn 6*/
char *BTF[BTFn] = { "Uncompr", "SQueezed", "LZW/1", "LZW/2", "Unix/12",
		"Unix/16" };

/* quick thread_format */
/*#define QTFn 6*/
char *QTF[QTFn] = { "unc", "squ", "shk", "sh2", "u12", "u16" };

/* file_sys_id */
/*#define FIDn 14*/
char *FID[FIDn] = { "Reserved/unknown ($00)", "ProDOS/SOS", "DOS 3.3",
		"DOS 3.2", "Apple II Pascal", "Macintosh (MFS)",
		"Macintosh (HFS)", "LISA file system", "Apple CP/M",
		"Reserved ($09)", "MS-DOS", "High-Sierra", "ISO 9660",
		"AppleShare" };

/* storage_type */
/*#define STn 14*/
char *ST[STn] = { "Standard file ($00)", "Standard file ($01)",
		"Standard file ($02)", "Standard file ($03)", "??? ($04)",
		"Extended file ($05)", "??? ($06)", "??? ($07)", "??? ($08)",
		"??? ($09)", "??? ($0a)", "??? ($0b)", "??? ($0c)",
		"Subdirectory ($0d)" };

/* file type names */
char *FT[256] = {
	"NON", "BAD", "PCD", "PTX", "TXT", "PDA", "BIN", "FNT",
	"FOT", "BA3", "DA3", "WPF", "SOS", "$0D", "$0E", "DIR",
	"RPD", "RPI", "AFD", "AFM", "AFR", "SCL", "PFS", "$17",
	"$18", "ADB", "AWP", "ASP", "$1C", "$1D", "$1E", "$1F",
	"TDM", "$21", "$22", "$23", "$24", "$25", "$26", "$27",
	"$28", "$29", "8SC", "8OB", "8IC", "8LD", "P8C", "$2F",
	"$30", "$31", "$32", "$33", "$34", "$35", "$36", "$37",
	"$38", "$39", "$3A", "$3B", "$3C", "$3D", "$3E", "$3F",
	"DIC", "OCR", "FTD", "$43", "$44", "$45", "$46", "$47",
	"$48", "$49", "$4A", "$4B", "$4C", "$4D", "$4E", "$4F",
	"GWP", "GSS", "GDB", "DRW", "GDP", "HMD", "EDU", "STN",
	"HLP", "COM", "CFG", "ANM", "MUM", "ENT", "DVU", "FIN",
	"$60", "$61", "$62", "$63", "$64", "$65", "$66", "$67",
	"$68", "$69", "$6A", "BIO", "$6C", "TDR", "PRE", "HDV",
	"$70", "$71", "$72", "$73", "$74", "$75", "$76", "$77",
	"$78", "$79", "$7A", "$7B", "$7C", "$7D", "$7E", "$7F",
	"$80", "$81", "$82", "$83", "$84", "$85", "$86", "$87",
	"$88", "$89", "$8A", "$8B", "$8C", "$8D", "$8E", "$8F",
	"$90", "$91", "$92", "$93", "$94", "$95", "$96", "$97",
	"$98", "$99", "$9A", "$9B", "$9C", "$9D", "$9E", "$9F",
	"WP ", "$A1", "$A2", "$A3", "$A4", "$A5", "$A6", "$A7",
	"$A8", "$A9", "$AA", "GSB", "TDF", "BDF", "$AE", "$AF",
	"SRC", "OBJ", "LIB", "S16", "RTL", "EXE", "PIF", "TIF",
	"NDA", "CDA", "TOL", "DVR", "LDF", "FST", "$BE", "DOC",
	"PNT", "PIC", "ANI", "PAL", "$C4", "OOG", "SCR", "CDV",
	"FON", "FND", "ICN", "$CB", "$CC", "$CD", "$CE", "$CF",
	"$D0", "$D1", "$D2", "$D3", "$D4", "MUS", "INS", "MDI",
	"SND", "$D9", "$DA", "DBM", "$DC", "DDD", "$DE", "$DF",
	"LBR", "$E1", "ATK", "$E3", "$E4", "$E5", "$E6", "$E7",
	"$E8", "$E9", "$EA", "$EB", "$EC", "$ED", "R16", "PAS",
	"CMD", "$F1", "$F2", "$F3", "$F4", "$F5", "$F6", "$F7",
	"$F8", "OS ", "INT", "IVR", "BAS", "VAR", "REL", "SYS" };


/*
 * NuView program
 */

/* print date from Time structure */
char *
PrintDate(tptr, brief)
Time *tptr;
int brief;
{
    static char buf[64];  /* holds final date string; must be static */
    char buf2[64];  /* space to hold string while building it */

    /* check for validity */
    if ( (tptr->day > 30) || (tptr->month > 11) || (tptr->hour > 24) ||
	(tptr->minute > 59) ) {
	strcpy(buf, "   <invalid>   ");
	return (buf);
    }

    if (!tptr->second && !tptr->minute && !tptr->hour && !tptr->day &&
	    !tptr->month && !tptr->year && !tptr->weekDay && !tptr->extra) {
	strcpy(buf, "   [No Date]   ");
	return (buf);
    }

    /* only print weekDay if one was stored and if we're in FULL mode */
    if (!brief && tptr->weekDay) {
	(void) sprintf(buf, "%s, ", WD[tptr->weekDay]);
    } else {
	buf[0] = '\0';
    }
    if (brief == 2) {	/* special case for ARCZOO format */
	(void) sprintf(buf2, "%.2d-%s-%.2d  %.2d:%.2d%c",
	    (tptr->day)+1, MO[tptr->month], tptr->year,
	    tptr->hour > 12 ? tptr->hour-12 : tptr->hour, tptr->minute,
	    tptr->hour > 12 ? 'p' : 'a');
    } else {
	(void) sprintf(buf2, "%.2d-%s-%.2d  %.2d:%.2d",
	    (tptr->day)+1, MO[tptr->month], tptr->year,
	    tptr->hour, tptr->minute);
    }
    (void) strcat(buf, buf2);
    if (!brief) {  /* add seconds to long output */
	(void) sprintf(buf2, ":%.2d", tptr->second);
	(void) strcat(buf, buf2);
    }
    return (buf);
}


/*
 * Dump contents of the threads (used by FULL view mode)
 */
static void
DumpThreads(RNodePtr)
RNode *RNodePtr;
{
    int i;
    fourbyt count = RNodePtr->RHptr->total_threads;
    static char ind[4] = "   ";  /* indentation */
    THblock *THptr;
    TNode *TNodePtr;

    /* go through all threads, printing as we go */
    TNodePtr = RNodePtr->TNodePtr;
    for (i = 0; (fourbyt) i < count; i++) {
	if (TNodePtr == (TNode *) NULL) {
	    fprintf(stderr, "WARNING: fewer threads than expected\n");
	    return;
	}
	THptr = TNodePtr->THptr;

	printf("%s --> Information for thread %d\n", ind, i);
	printf("%s thread_class: %s\n", ind, THptr->thread_class < TCn ?
		TC[THptr->thread_class] : unknownStr);
	printf("%s thread_format: %s\n", ind, THptr->thread_format < TFn ?
		TF[THptr->thread_format] : unknownStr);
	printf("%s thread_kind: %s ($%.2X)\n", ind,
		(THptr->thread_kind < TKn && THptr->thread_class < TCn) ?
		TK[THptr->thread_class][THptr->thread_kind] : unknownStr,
		THptr->thread_kind);
	printf("%s thread_crc: $%.4x\n", ind, THptr->thread_crc);
	printf("%s thread_eof: %lu   ", ind, THptr->thread_eof);
	printf("comp_thread_eof: %lu\n", THptr->comp_thread_eof);
	printf("%s * position within file: %ld\n", ind, TNodePtr->fileposn);

	TNodePtr = TNodePtr->TNext;
    }
    /* after all info printed, show sum total of thread lengths */
    printf("%s * total thread_eof: %lu   ", ind, RNodePtr->unc_len);
    printf("total comp_thread_eof: %lu\n", RNodePtr->comp_len);
}


/*
 * Scan contents of the threads for certain things (for PROSHK view mode)
 * Returns 65535 as error code (-1 in an unsigned short).
 * Places the format, compressed EOF, and uncompressed EOF in the location
 * pointed to by the appropriate variables.
 *
 * This will probably fail if there are > 32767 threads.
 */
static twobyt
ScanThreads(RNodePtr, format, dataCEOF, dataEOF)
RNode *RNodePtr;
twobyt *format;  /* format of the data_fork thread */
long *dataCEOF;  /* length of the data_fork thread (compressed) */
long *dataEOF;  /* length of the data_fork thread (uncompressed) */
{
    int i;
    int count;
    THblock *THptr;
    TNode *TNodePtr;

    count = (int) RNodePtr->RHptr->total_threads;
    *format = 65535;  /* default = error */
    *dataCEOF = 0L;
    *dataEOF = 0L;
    TNodePtr = RNodePtr->TNodePtr;
    for (i = 0; i < count; i++) {
	if (TNodePtr == (TNode *) NULL) {
	    fprintf(stderr, "WARNING: fewer threads than expected\n");
	    return (65535);
	}
	THptr = TNodePtr->THptr;

	if (THptr->thread_class == 2) {  /* data thread? */
	    *format = THptr->thread_format;
	    *dataCEOF = THptr->comp_thread_eof;
	    *dataEOF = THptr->thread_eof;
	    return (THptr->thread_kind);
	}
	TNodePtr = TNodePtr->TNext;
    }
    return (65535);  /* no data thread found */
}


/*
 * View archive contents
 *
 * Format types:
 *	T: NAMEONLY - Brief output of filenames only (good for pipes)
 *	V: PROSHK - ProDOS ShrinkIt format
 *	A: ARCZOO - Format similar to ARC or ZOO
 *	Z: FULL - Fully detailed output
 */
void
NuView(filename, options)
char *filename;
char *options;
{
    ListHdr *archive;
    MHblock *MHptr;
    RHblock *RHptr;
    RNode *RNodePtr;
    outtype prtform;
    int rec;
    char tmpbuf[80];  /* temporary buffer for sprintf + printf */
    twobyt format, datakind;  /* PROSHK */
    int percent;  /* PROSHK & ARCZOO */
    long dataCEOF, dataEOF;  /* PROSHK */
    long total_files = 0L, total_length = 0L, total_complen = 0L;  /* ARCZOO */

#ifdef APW  /* kill "not used" messages */
    char *ptr;
#endif
    static char *procName = "NuView";

    /* process options ourselves */
    switch (options[0]) {
    case 't':
	if (INDEX(options+1, 'v')) prtform = PROSHK;  /* -tv is same as -v */
	else if (INDEX(options+1, 'a')) prtform = ARCZOO;
	else if (INDEX(options+1, 'z')) prtform = FULL;
	else prtform = NAMEONLY;
	break;
    case 'v':
	prtform = PROSHK;
	break;
    default:
	fprintf(stderr, "%s internal error: unknown output format\n", prgName);
	Quit (-1);
    }

    archive = NuRead(filename);
    MHptr = archive->MHptr;

    /* Print master header info */
    if (prtform == NAMEONLY) {
	/* don't print any info from master header for NAMEONLY */
    } else if (prtform == PROSHK) {
#ifdef APW
	/* strip partial paths from APW filename (if any) */
	ptr = RINDEX(archive->arc_name, '/');
	printf(" %-15.15s ", ptr ? ptr+1 : archive->arc_name);
#else
	printf(" %-15.15s ", archive->arc_name);
#endif
	printf("Created:%s   ", PrintDate(&MHptr->arc_create_when, TRUE));
	printf("Mod:%s     ", PrintDate(&MHptr->arc_mod_when, TRUE));
	printf("Recs:%5lu\n\n", MHptr->total_records);
	printf(" Name                  Kind  Typ  Auxtyp Archived");
	printf("         Fmat Size Un-Length\n");
	printf("-------------------------------------------------") ;
	printf("----------------------------\n");
    } else if (prtform == ARCZOO) {
	printf("Name                      Length    Stowage    SF   Size now");
	printf("  Date       Time  \n");
	printf("========================  ========  ========  ====  ========");
	printf("  =========  ======\n");
    } else if (prtform == FULL) {
	printf("Now processing archive '%s'\n", archive->arc_name);
	printf("---> Master header information:\n");
	printf("master ID: '%.6s'   ", MHptr->ID);
	printf("master_version: $%.4x   ", MHptr->master_version);
	printf("master_crc: $%.4X\n", MHptr->master_crc);
	printf("total_records: %lu    ", MHptr->total_records);
	if (MHptr->master_version >= 0x0001) {
	    printf("master_eof: %lu\n", MHptr->master_eof);
	} else {
	    printf("\n");
	}
	printf("created: %s   ", PrintDate(&MHptr->arc_create_when, FALSE));
	printf("mod: %s\n", PrintDate(&MHptr->arc_mod_when, FALSE));
    } else {
	printf("NuView internal error: undefined output format\n");
	Quit (-1);
    }

    /* Print record info */
    RNodePtr = archive->RNodePtr;
    for (rec = 0; (fourbyt) rec < MHptr->total_records; rec++) {
	if (RNodePtr == (RNode *) NULL) {
	    fprintf(stderr, "WARNING: fewer records than expected\n");
	    return;
	}
	RHptr = RNodePtr->RHptr;

	if (prtform == NAMEONLY) {
	    printf("%.79s\n", RNodePtr->filename);  /* max 79 chars */
	} else if (prtform == PROSHK) {
	    printf("%c", (RHptr->access == 0xE3L || RHptr->access == 0xC3L) ?
		' ' : '+');
	    printf("%-21.21s ", RNodePtr->filename);
	    /* get info on data_fork thread */
	    datakind = ScanThreads(RNodePtr, &format, &dataCEOF, &dataEOF);
	    if (datakind == 65535) {  /* no data thread... */
		printf("????  ");
		printf("%s  ", RHptr->file_type < 256L ? FT[RHptr->file_type] :
			"???");
		printf("$%.4X  ", (twobyt) RHptr->extra_type);
	    } else if (datakind == 1) {  /* disk */
		printf("Disk  ");
		printf("---  ");
		(void) sprintf(tmpbuf, "%dk", (twobyt) RHptr->extra_type / 2);
		printf("%-5s  ", tmpbuf);
	    } else {  /* must be a file */
		printf("File  ");
		printf("%s  ", RHptr->file_type < 256L ? FT[RHptr->file_type] :
			"???");
		printf("$%.4X  ", (twobyt) RHptr->extra_type);
	    }
	    printf("%s  ", PrintDate(&RHptr->archive_when, TRUE));
	    printf("%s  ", format < QTFn ? QTF[format] : "???");

	    /* figure out the percent size, and format it appropriately */
	    /* Note RNodePtr->comp_len corresponds to dataCEOF, and */
	    /*      RNodePtr->unc_len corresponds to dataEOF.       */
	    if (!dataCEOF && !dataEOF) {
		printf("100%%  ");  /* file is 0 bytes long */
	    } else if ((!dataEOF && dataCEOF) || (dataEOF && !dataCEOF)) {
		printf("---   ");  /* something weird happened */
	    } else if (dataEOF < dataCEOF) {
		printf(">100%% ");  /* compression failed?!? */
	    } else {  /* compute from sum of thread lengths (use only data?) */
		percent = (dataCEOF * 100) / dataEOF;
		(void) sprintf(tmpbuf, "%.2d%%", percent);
		printf("%-4s  ", tmpbuf);
	    }
	    if (!dataEOF && dataCEOF)  /* weird */
		printf("   ????\n");
	    else
		printf("%7ld\n", dataEOF);	/* was 8ld */
	} else if (prtform == ARCZOO) {
	    printf("%-24.24s  ", RNodePtr->filename);
	    datakind = ScanThreads(RNodePtr, &format, &dataCEOF, &dataEOF);
	    printf("%8ld  ", dataEOF);
	    printf("%-8.8s  ", format < BTFn ? BTF[format] : "Unknown");

	    /* figure out the percent size, and format it appropriately */
	    /* Note RNodePtr->comp_len corresponds to dataCEOF, and */
	    /*      RNodePtr->unc_len corresponds to dataEOF.       */
	    if (!dataCEOF && !dataEOF) {
		printf("  0%%  ");  /* file is 0 bytes long */
	    } else if ((!dataEOF && dataCEOF) || (dataEOF && !dataCEOF)) {
		printf("---   ");  /* something weird happened */
	    } else if (dataEOF < dataCEOF) {
		printf(" <0%%  ");  /* compression failed?!? */
	    } else {  /* compute from sum of thread lengths (use only data?) */
		percent = 100 - ((dataCEOF * 100) / dataEOF);
		if (percent == 0 || percent == 100)
		    (void) sprintf(tmpbuf, "%d%%", percent);
		else
		    (void) sprintf(tmpbuf, "%.2d%%", percent);
		printf("%4s  ", tmpbuf);
	    }
	    printf("%8ld  ", dataCEOF);
	    printf("%s\n", PrintDate(&RHptr->mod_when, 2));

	    total_files++;
	    total_length += dataEOF;
	    total_complen += dataCEOF;
	} else if (prtform == FULL) {
	    printf("\n---> Information for record %d:\n", rec);
	    printf("Filename: (%d) '%s'\n",
				RNodePtr->filename_length, RNodePtr->filename);
	    printf("header ID: '%.4s'   ", RHptr->ID);
	    printf("version_number: $%.4X   ", RHptr->version_number);
	    printf("header_crc: $%.4X\n", RHptr->header_crc);
	    printf("attrib_count: %u   ", RHptr->attrib_count);
	    printf("total_threads: %u\n", RHptr->total_threads);
	    printf("file_sys_id: %s   ", RHptr->file_sys_id < FIDn ?
		FID[RHptr->file_sys_id] : unknownStr);
	    printf("sep: '%c'\n", RHptr->file_sys_info);
	    if (RHptr->file_sys_id == 0x0001) {  /* ProDOS-specific */
		printf("access: %s ($%.8lX)   ", (RHptr->access == 0xE3L ||
		RHptr->access == 0xC3L) ? "Unlocked" : "Locked", RHptr->access);
		printf("file_type: %s ($%.8lX)\n", RHptr->file_type < 256L ?
		    FT[RHptr->file_type] : "???", RHptr->file_type);
	    } else {  /* all other filesystems */
		printf("access: $%.8lX   ", RHptr->access);
		printf("file_type: $%.8lX\n", RHptr->file_type);
	    }
	    printf("extra_type: $%.8lX   ", RHptr->extra_type);
	    printf("storage_type: %s\n", RHptr->storage_type < STn ?
		ST[RHptr->storage_type] : unknownStr);
	    printf("created: %s   ", PrintDate(&RHptr->create_when, FALSE));
	    printf("mod: %s\n", PrintDate(&RHptr->mod_when, FALSE));
	    printf("archived: %s\n", PrintDate(&RHptr->archive_when,
		    FALSE));
	    printf("GS/OS option_size: %.4x\n", RHptr->option_size);
	    /* future expansion... */
	} else {
	    printf("%s internal error: undefined output format\n", prgName);
	    Quit (-1);
	}

	/* Print thread info */
	if (prtform == FULL) DumpThreads(RNodePtr);
	RNodePtr = RNodePtr->RNext;  /* advance to next record */
#ifdef APW
	if (STOP()) Quit (1);  /* check for OA-period */
#endif
    }

    /* end of archive processing */
    if (prtform == ARCZOO) {
	printf(
	    "                     ===  ========            ====  ========\n");
	printf("Total                ");
	printf("%3ld  ", total_files);
	printf("%8ld            ", total_length);

	/* figure out the percent size, and format it appropriately */
	if (!total_complen && !total_length) {
	    printf("  0%%  ");  /* file is 0 bytes long */
	} else if ((!total_length && total_complen) ||
					(total_length && !total_complen)) {
	    printf("---   ");  /* something weird happened */
	} else if (total_length < total_complen) {
	    printf(" <0%%  ");  /* compression failed?!? */
	} else {  /* compute from sum of thread lengths (use only data?) */
	    percent = 100 - (int) ((total_complen * 100L) / total_length);
	    if (percent == 0 || percent == 100)
		(void) sprintf(tmpbuf, "%d%%", percent);
	    else
		(void) sprintf(tmpbuf, "%.2d%%", percent);
	    printf("%4s  ", tmpbuf);
	}

	printf("%8ld\n", total_complen);

    } else if (prtform == FULL) {
	printf("\n*** end of file position: %ld\n", archive->nextposn);
    }  /* else do nothing */
}

