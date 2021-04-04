/*
 * nupak.h - declarations for nupak.c
 *
 * NuLib v3.2  March 1992  Freeware (distribute, don't sell)
 * By Andy McFadden (fadden@uts.amdahl.com)
 */
/*
 * This has function declarations for all of the pack routines; that way we
 *   don't have to include .h files for all of the pack code.
 */

/* Pack/unpack buffer size; should be as big as read() & malloc() can take */
/* Note: must be AT LEAST 8200 bytes or things may break */
/*       Bad things could happen if it's less than 12K   */
#define PAKBUFSIZ   0xff80

extern long packedSize;
extern onebyt lastseen;

extern twobyt PackFile();
extern int UnpackFile();  /* BOOLEAN */
extern unsigned int crlf();
extern void Spin(),
	    FCopy();

extern long pak_SHK();    /* pack P8 ShrinkIt format, in nushk.c */
extern void unpak_SQU();  /* unsqueeze, in nusq.c */
extern void unpak_SHK();  /* unShrink, in nushk.c */

