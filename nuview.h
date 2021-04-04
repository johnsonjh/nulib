/*
 * nuview.h - declarations for nuview.c
 *
 * NuLib v3.2  March 1992  Freeware (distribute, don't sell)
 * By Andy McFadden (fadden@uts.amdahl.com)
 */

typedef enum { NAMEONLY, PROSHK, ARCZOO, FULL } outtype;

/* constant string declarations */
extern char *unknownStr;
extern char *WD[];
extern char *MO[];
#define TCn 4
extern char *TC[];
#define TKn 3
extern char *TK[][TKn];
#define TFn 6
extern char *TF[];
#define BTFn 6
extern char *BTF[];
#define QTFn 6
extern char *QTF[];
#define FIDn 14
extern char *FID[];
#define STn 14
extern char *ST[];
extern char *FT[];

extern void NuView();
extern char *PrintDate();

