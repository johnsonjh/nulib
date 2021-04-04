/*
 * nuadd.h - declarations for nuadd.c
 *
 * NuLib v3.2  March 1992  Freeware (distribute, don't sell)
 * By Andy McFadden (fadden@uts.amdahl.com)
 */

/* information is copied from file-dependent structures (FileRec) to here */
typedef struct {
    char *pathname;	 /* as much of the path as we need to know */
    char *store_name;  /* what name the file will be stored under */
    fourbyt eof;  /* length of file */
    fourbyt fAccess;  /* was Word */
    fourbyt fileType;  /* was Word */
    fourbyt auxType;
    twobyt storageType;
    Time create_dt;  /* Time = TimeRec = 8 bytes in misctool.h/nuread.h */
    Time mod_dt;
    twobyt fileSysID;  /* these two are non-standard */
    onebyt fileSysInfo;
    int marked;  /* application specific */
} file_info;

#define MAXARGS 255  /* max #of files specified on command line; signed int */

extern void NuAdd();
extern long AddFile();
extern onebyt *MakeMHblock();
extern int  EvalArgs();

