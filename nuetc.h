/*
 * nuetc.h - declarations for nuetc.c
 *
 * NuLib v3.2  March 1992  Freeware (distribute, don't sell)
 * By Andy McFadden (fadden@uts.amdahl.com)
 */
/*
 * (this will be included by almost all source files; it should come last)
 */

/* define these if they haven't been already */
/* (typedef int BOOLEAN caused too many problems... #define is easier) */
#ifndef BOOLEAN
# define BOOLEAN int
#endif

#ifndef TRUE
# define TRUE	 1
# define FALSE	 0
#endif

#ifdef UNIX
# ifdef BSD43
#  ifndef NeXT
   extern char *index();    /* BSD version */
   extern char *rindex();
#  endif
#  define INDEX(s, c)  index(s, c)
#  define RINDEX(s, c) rindex(s, c)
# else
   extern char *strchr();   /* AT&T version */
   extern char *strrchr();
#  define INDEX(s, c)  strchr(s, c)
#  define RINDEX(s, c) strrchr(s, c)
# endif
#else
  extern char *strchr();    /* APW, MSC */
  extern char *strrchr();
# define INDEX(s, c)  strchr(s, c)
# define RINDEX(s, c) strrchr(s, c)
#endif

extern char tmpNameBuf[];

/* external function declarations */
extern void Fatal(),
	    Quit();
extern void free();
extern char *Malloc();

#ifdef APW
extern void ToolErrChk(),
	    perror();
#endif

extern int strcasecmp(),
	   strncasecmp();

extern void ArcfiCreate(),
	    Rename();
extern BOOLEAN Exists();
extern char *MakeTemp();

extern void ExpandTime();
extern long ReduceTime();
extern Time *GetTime();

