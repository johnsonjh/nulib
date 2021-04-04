/*
 * apwerr.h - text versions of APW and ProDOS 16 error codes
 *
 * NuLib v3.2  March 1992  Freeware (distribute, don't sell)
 * By Andy McFadden (fadden@uts.amdahl.com)
 */
/*
 * (ERROR() didn't cut it, and I'm trying to separate things from the shell.)
 */

/* APW-specific UNIX-like errors */

/*
 [ this is derived from: ]
 errno.h -- error return codes

 Copyright American Telephone & Telegraph
 Modified and used with permission, Apple Computer Inc.
 Copyright Apple Computer Inc. 1985, 1986, 1987
 All rights reserved.
*/
/* @(#)errno.h 2.1 */
/* 3.0 SID # 1.3 */

#define sys_nerr 35	   /* err must be < Max APW Err */
static char *sys_errlist[sys_nerr] = {
    /* 0  (no err)  */	"[ call successful ]",
    /* 1  EPERM     */	"permission denied",
    /* 2  ENOENT    */	"no such file or directory",
    /* 3  ENORSRC   */	"no such resource",
    /* 4  EINTR     */	"interrupted system call",
    /* 5  EIO	    */	"I/O error",
    /* 6  ENXIO     */	"no such device or address",
    /* 7  E2BIG     */	"insufficient space for return argument",
    /* 8  ENOEXEC   */	"exec format error",
    /* 9  EBADF     */	"bad file number",
    /* 10 ECHILD    */	"no children",
    /* 11 EAGAIN    */	"no more processes",
    /* 12 ENOMEM    */	"not enough memory",
    /* 13 EACCES    */	"permission denied",
    /* 14 EFAULT    */	"bad address",
    /* 15 ENOTBLK   */	"block device required",
    /* 16 EBUSY     */	"mount device busy",
    /* 17 EEXIST    */	"file exists",
    /* 18 EXDEV     */	"cross-device link",
    /* 19 ENODEV    */	"no such device",
    /* 20 ENOTDIR   */	"not a directory",
    /* 21 EISDIR    */	"is a directory",
    /* 22 EINVAL    */	"invalid argument",
    /* 23 ENFILE    */	"file table overflow",
    /* 24 EMFILE    */	"too many open files",
    /* 25 ENOTTY    */	"not a typewriter (sorry)",
    /* 26 ETXTBSY   */	"text file busy",
    /* 27 EFBIG     */	"file too large",
    /* 28 ENOSPC    */	"no space left on device",
    /* 29 ESPIPE    */	"illegal seek",
    /* 30 EROFS     */	"read only file system",
    /* 31 EMLINK    */	"too many links",
    /* 32 EPIPE     */	"broken pipe",
    /* 33 EDOM	    */	"math arg out of domain of func",
    /* 34 ERANGE    */	"math result not representable"
};


/* ProDOS errors */

/* [ This is derived from: ]
/********************************************
; File: ProDos.h
;
;
; Copyright Apple Computer, Inc. 1986, 1987
; All Rights Reserved
;
********************************************/

#define MPErr 0x61	/* err must be < Max ProDOS Err # */
static char *ProDOSErr[MPErr] = {
    /* 00 (no error)	    */ "[ ProDOS call successful ]",
    /* 01 invalidCallNum    */ "invalid call number / (fatal) unclaimed intr",
    /* 02		    */ "",
    /* 03		    */ "",
    /* 04		    */ "(ProDOS 8 invalid parameter count)",
    /* 05 badPBlockPtr	    */ "call pointer out of bounds",
    /* 06 pdosActiveErr     */ "ProDOS is active",
    /* 07 pdosBusyErr	    */ "ProDOS is busy",
    /* 08		    */ "",
    /* 09		    */ "",
    /* 0a vcbUnusable	    */ "(fatal) VCB is unusable",
    /* 0b fcbUnusable	    */ "(fatal) FCB is unusable",
    /* 0c badBlockZero	    */ "(fatal) block zero allocated illegally",
    /* 0d shdwInterruptErr  */ "(fatal) interrupt occurred while I/O shadowing off",
    /* 0e		    */ "",
    /* 0f		    */ "",
    /* 10 devNotFound	    */ "device not found",
    /* 11 badDevRefNum	    */ "invalid device ref# / (fatal) wrong OS version",
    /* 12		    */ "",
    /* 13		    */ "",
    /* 14		    */ "",
    /* 15		    */ "",
    /* 16		    */ "",
    /* 17		    */ "",
    /* 18		    */ "",
    /* 19		    */ "",
    /* 1a		    */ "",
    /* 1b		    */ "",
    /* 1c		    */ "",
    /* 1d		    */ "",
    /* 1e		    */ "",
    /* 1f		    */ "",
    /* 20 badReqCode	    */ "invalid request code",
    /* 21		    */ "",
    /* 22		    */ "",
    /* 23		    */ "",
    /* 24		    */ "",
    /* 25 intTableFull	    */ "interrupt table full",
    /* 26 invalidOperation  */ "invalid operation",
    /* 27 ioError	    */ "I/O error",
    /* 28 noDevConnect	    */ "no device connected",
    /* 29		    */ "",
    /* 2a		    */ "",
    /* 2b writeProtectErr   */ "write protect error",
    /* 2c		    */ "",
    /* 2d		    */ "",
    /* 2e diskSwitchErr     */ "disk switched error",
    /* 2f		    */ "device not online",
    /* 30		    */ "device-specific err $30",
    /* 31		    */ "device-specific err $31",
    /* 32		    */ "device-specific err $32",
    /* 33		    */ "device-specific err $33",
    /* 34		    */ "device-specific err $34",
    /* 35		    */ "device-specific err $35",
    /* 36		    */ "device-specific err $36",
    /* 37		    */ "device-specific err $37",
    /* 38		    */ "device-specific err $38",
    /* 39		    */ "device-specific err $39",
    /* 3a		    */ "device-specific err $3a",
    /* 3b		    */ "device-specific err $3b",
    /* 3c		    */ "device-specific err $3c",
    /* 3d		    */ "device-specific err $3d",
    /* 3e		    */ "device-specific err $3e",
    /* 3f		    */ "device-specific err $3f",
    /* 40 badPathName	    */ "invalid pathname syntax",
    /* 41		    */ "",
    /* 42 fcbFullErr	    */ "FCB full error (too many files open)",
    /* 43 badFileRefNum     */ "invalid file reference number",
    /* 44 pathNotFound	    */ "path not found",
    /* 45 volumeNotFound    */ "volume not found",
    /* 46 fileNotFound	    */ "file not found",
    /* 47 dupFileName	    */ "duplicate file name",
    /* 48 volumeFullErr     */ "volume full error",
    /* 49 dirFullErr	    */ "directory full error",
    /* 4a versionErr	    */ "version error (incompatible file format)",
    /* 4b badStoreType	    */ "unsupported (or incorrect) storage type",
    /* 4c eofEncountered    */ "end-of-file encountered",
    /* 4d positionRangeErr  */ "position out of range",
    /* 4e accessErr	    */ "access not allowed",
    /* 4f		    */ "",
    /* 50 fileOpenErr	    */ "file already open",
    /* 51 dirDamaged	    */ "directory structure is damaged (file count?)",
    /* 52 badVolType	    */ "unsupported volume type",
    /* 53 paramRangeErr     */ "parameter out of range",
    /* 54 memoryFullErr     */ "out of memory",
    /* 55 vcbFullErr	    */ "VCB full error",
    /* 56		    */ "(ProDOS 8 bad buffer address)",
    /* 57 dupVolumeErr	    */ "duplicate volume error",
    /* 58 notBlkDevErr	    */ "not a block device",
    /* 59 invalidLevel	    */ "invalid level",

    /* 5a		    */ "block number out of range (bad vol bitmap?)",
    /* 5b		    */ "illegal pathname change",
    /* 5c		    */ "not an executable file",
    /* 5d		    */ "file system not available",
    /* 5e		    */ "cannot deallocate /RAM",
    /* 5f		    */ "return stack overflow",
    /* 60		    */ "data unavailable"
};
