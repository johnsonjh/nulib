/*
 * nucompfn.h - function declarations for nucomp.c
 *
 * NuLib v3.2  March 1992  Freeware (distribute, don't sell)
 * By Andy McFadden (fadden@uts.amdahl.com)
 */

extern int u_compress(),
	   u_decompress();

/*  COMPRESS.FNS  global function declarations */
/*  this should be compatible with any type of declaration for external
    functions. See compress.h for explaination */
#ifdef  NPROTO
extern  void Usage();
extern  int check_error();
extern  char *name_index();
extern  char *get_program_name();
#ifdef NO_STRCHR
extern char *strchr();
#endif
#ifdef NO_STRRCHR
extern char *strrchr();
#endif
#ifdef NO_REVSEARCH
extern char *strrpbrk();
#endif
extern  int is_z_name();
extern  int cl_block();
extern  int make_z_name();
extern  void unmake_z_name();
extern  void compress();
extern  void putcode();
extern  void decompress();
extern  CODE getcode();
extern  void writeerr();
extern  void copystat();
#ifndef NOSIGNAL
extern  int foreground();
extern  SIGTYPE onintr();
extern  SIGTYPE oops();
#endif
extern  void prratio();
extern  void version();
#ifdef NEARHEAP
extern ALLOCTYPE *emalloc();
extern void efree();
#else
extern  ALLOCTYPE FAR *emalloc();
extern  void efree();
#endif
extern  int alloc_tables();
extern  void init_tables();
extern  int nextcode();
#else
extern  void Usage(int);
extern  int  check_error(void);
extern  char *name_index(char *);
extern  int cl_block(void);
extern  char *get_program_name(char *);
extern  int is_z_name(char *);
extern  int make_z_name(char *);
extern  void unmake_z_name(char *);
#ifdef NO_STRCHR
extern char *strchr(char *,int);
#endif
#ifdef NO_STRRCHR
extern char *strrchr(char *,int);
#endif
#ifdef NO_REVSEARCH
extern char *strrpbrk(char *,char *);
#endif
extern  void compress(void);
extern  void putcode(CODE,int);
extern  void decompress(void);
extern  CODE getcode(void);
extern  void writeerr(void);
extern  void copystat(char *,char *);
#ifndef NOSIGNAL
extern  int foreground(void);
extern  SIGTYPE onintr(void);
extern  SIGTYPE oops(void);
#endif
extern  void prratio(FILE *,long,long);
extern  void version(void);
#ifdef NEARHEAP
extern ALLOCTYPE *emalloc(unsigned int,int);
extern void efree(ALLOCTYPE *);
#else
extern  ALLOCTYPE FAR *emalloc(unsigned int,int);
extern  void efree(ALLOCTYPE FAR *);
#endif
extern  int alloc_tables(CODE,HASH);
extern  void init_tables(void );
extern  int nextcode(CODE *);
#endif

