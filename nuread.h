/*
 * nuread.h - linked list structures used for holding NuFX header data,
 *	  and structure definitions for archive innards
 *
 * NuLib v3.2  March 1992  Freeware (distribute, don't sell)
 * By Andy McFadden (fadden@uts.amdahl.com)
 */
/*
 * (this will be included by all source files which access NuFX archives)
 */


/* The NuFX master format version we output, and the maximum we can extract */
#define OURMVERS 2
#define MAXMVERS 2

/* The NuFX record format version we output, and the maximum we can extract */
#define OURVERS 0
#define MAXVERS 3

/* "NuFile" in alternating ASCII */
static onebyt MasterID[7] = { 0x4e, 0xf5, 0x46, 0xe9, 0x6c, 0xe5, 0x0 };

/* "NuFX" in alternating ASCII */
static onebyt RecordID[5] = { 0x4e, 0xf5, 0x46, 0xd8, 0x0 };


/*
 * Structure definitions for NuFX innards
 */

/* master header block */
typedef struct {
    onebyt ID[6];
    twobyt master_crc;
    fourbyt total_records;
    Time arc_create_when;
    Time arc_mod_when;
    twobyt master_version;
    onebyt reserved1[8];
    fourbyt master_eof;
    onebyt reserved2[6];
} MHblock;
#define MHsize 48  /* this should not change */

/* record header block */
typedef struct {
    onebyt ID[4];
    twobyt header_crc;
    twobyt attrib_count;
    twobyt version_number;
    twobyt total_threads;
    twobyt reserved1;
    twobyt file_sys_id;
    onebyt file_sys_info;
    onebyt reserved2;
    fourbyt access;
    fourbyt file_type;
    fourbyt extra_type;
    twobyt storage_type;
    Time create_when;
    Time mod_when;
    Time archive_when;
    twobyt option_size;
    /* future expansion here... */
} RHblock;
#define RHsize	58	/* sizeof(RHblock) should work, but might not */
#define ATTSIZE 64	/* default attrib_count when creating new */

/*
 * This buffer must be able to contain three things (not all at once):
 * - The master header block (size = MHsize)
 * - The record header block (size = RHsize)
 * - Attributes not specified in the RHblock (attrib_count - RHsize - 2)
 *
 * Currently, it only needs to be 64 bytes.  Since it is allocated as local
 *  storage only once during execution, making it reasonably large should
 *  not cause any problems in performance but will make the program stable
 *  if it encounters an archive with a drastically expanded RHblock.
 */
#define RECBUFSIZ	256

/* thread record */
typedef struct {
    twobyt thread_class;
    twobyt thread_format;
    twobyt thread_kind;
    twobyt thread_crc;
    fourbyt thread_eof;
    fourbyt comp_thread_eof;
} THblock;
#define THsize 16  /* this should not change */


/*
 * Definitions for the linked lists
 * A linked list of Record headers, with linked lists of Threads attached
 */

/* thread nodes */
typedef struct TNode_s {
    THblock *THptr;  /* points to thread info */
    long fileposn;  /* absolute position of this thread in the file */
    struct TNode_s *TNext;  /* points to next thread node */
} TNode;

/* record nodes */
typedef struct RNode_s {
    RHblock *RHptr;  /* points to the record header block */
    char *filename;  /* filename of record */
    twobyt filename_length;  /* length of filename (as stored in record hdr) */
    twobyt real_fn_length;   /* length of filename (actual) */
    TNode *TNodePtr;  /* points to first thread node */
    fourbyt unc_len;  /* total uncompressed length of all threads */
    fourbyt comp_len;  /* total compressed length of all threads */
    struct RNode_s *RNext;  /* points to next record node */
} RNode;

/* head of list */
typedef struct {
    char *arc_name;  /* filename of archive */
    MHblock *MHptr;  /* points to master header */
    RNode *RNodePtr;  /* points to first record node */
    long nextposn;  /* abs. position in file to put next record (for ADD) */
} ListHdr;


/*
 * function declarations
 */

extern ListHdr *NuRead();  /* read archive info into memory */
extern void NuTest();	   /* archive integrity check */
extern twobyt CalcCRC();   /* calculate a CRC on a range of bytes */
extern char *PrintDate();  /* print a date from a (Time *) struct */
extern void BCopy();	   /* copy bytes: *src, *dest, num, order? */
extern void HiSwap();	   /* swap bytes (maybe): *ptr, src_index, dst_index */
