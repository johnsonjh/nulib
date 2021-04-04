/* This is part of the iostream/stdio library, providing -*- C -*- I/O.
   Define ANSI C stdio on top of C++ iostreams.
   Copyright (C) 1991 Per Bothner.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.


This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free
Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifndef _STDIO_H
#define _STDIO_H
#define _STDIO_USES_IOSTREAM

#include <features.h>

#ifndef NULL
#ifdef __cplusplus
#define NULL	0
#else
#define NULL	((void *)0)
#endif
#endif

#ifndef EOF
#define EOF (-1)
#endif
#ifndef BUFSIZ
#define BUFSIZ 1024
#endif

/* check streambuf.h */
#define STDIO_S_EOF_SEEN 16		/* _S_EOF_SEEN */
#define STDIO_S_ERR_SEEN 32		/* _S_ERR_SEEN */

#define _IOFBF 0 /* Fully buffered. */
#define _IOLBF 1 /* Line buffered. */
#define _IONBF 2 /* No buffering. */

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#include "_G_config.h"
#ifdef __STDC__
#ifdef	_G_NEED_STDARG_H
#include <stdarg.h>
#endif
#endif

/* define size_t.  Crud in case <sys/types.h> has defined it. */
#if !defined(_SIZE_T) && !defined(_T_SIZE_) && !defined(_T_SIZE) && \
	!defined(__SIZE_T) && !defined(_SIZE_T_) && \
	!defined(_SIZET_) && !defined(___int_size_t_) && \
	!defined(_GCC_SIZE_T)
#define _SIZE_T
#define _T_SIZE_
#define _T_SIZE
#define __SIZE_T
#define _SIZET_
#define _SIZE_T_
#define ___int_size_t_h
#define _GCC_SIZE_T
typedef _G_size_t size_t;
#endif

#ifndef _FPOS_T
#define _FPOS_T
typedef _G_fpos_t fpos_t;
#endif

#ifdef __linux__
#define TMP_MAX         238328
#endif

#define FOPEN_MAX	_G_FOPEN_MAX
#define FILENAME_MAX	_G_FILENAME_MAX

#define L_ctermid     9
#define L_cuserid     9
#define P_tmpdir      "/usr/tmp/"
#define L_tmpnam      25

struct __FILE {
    /* NOTE: Must match (or be a prefix of) __streambuf! */
    int _flags;		/* High-order word is _IO_MAGIC; rest is flags. */
    char* _gptr;	/* Current get pointer */
    char* _egptr;	/* End of get area. */
    char* _eback;	/* Start of putback+get area. */
    char* _pbase;	/* Start of put area. */
    char* _pptr;	/* Current put pointer. */
    char* _epptr;	/* End of put area. */
    char* _base;	/* Start of reserve area. */
    char* _ebuf;	/* End of reserve area. */
    struct streambuf *_chain;
};

typedef struct __FILE FILE;

/* For use by debuggers. These are linked in if printf or fprintf
 * are used. */
extern FILE *stdin, *stdout, *stderr; /* For use by debuggers. */

//extern struct _fake_filebuf __std_filebuf_0, __std_filebuf_1,
//		__std_filebuf_2;
//#define stdin ((FILE*)&__std_filebuf_0)
//#define stdout ((FILE*)&__std_filebuf_1)
//#define stderr ((FILE*)&__std_filebuf_2)

__BEGIN_DECLS

extern void	clearerr __P((FILE*));
extern char*	ctermid __P((char *));
extern char*	cuserid __P((char *));
extern int	fclose __P((FILE*));
extern int	feof __P((FILE*));
extern int	ferror __P((FILE*));
extern int	fflush __P((FILE*));
extern int	fgetc __P((FILE *));
extern int	fgetpos __P((FILE *, fpos_t *));
extern char*	fgets __P((char*, int, FILE*));
extern FILE*	fopen __P((__const char*, __const char*));
extern int	fprintf __P((FILE*, __const char* __format, ...));
extern int	fputc __P((int, FILE*));
extern int	fputs __P((__const char *str, FILE *fp));
extern size_t	fread __P((void*, size_t, size_t, FILE*));
extern FILE*	freopen __P((__const char*, __const char*, FILE*));
extern int	fscanf __P((FILE *__fp, __const char* __format, ...));
extern int	fseek __P((FILE* __fp, long int __offset, int __whence));
extern int	fsetpos __P((FILE *, __const fpos_t *));
extern long int	ftell __P((FILE* fp));
extern size_t	fwrite __P((__const void*, size_t, size_t, FILE*));
extern int	getc __P((FILE *stream));
extern int	getchar __P((void));
extern char*	gets __P((char*));
extern int	getw __P((FILE*));
extern int	printf __P((__const char* __format, ...));
extern int	putc __P((int __c, FILE*));
extern int	putchar __P((int __c));
extern int	puts __P((__const char*));
extern int	putw __P((int, FILE*));
extern int	remove __P((__const char*));
/* extern int	rename __P ((__const char *__old, __const char *__new)); */
extern void	rewind __P((FILE*));
extern int	scanf __P((__const char* __format, ...));
extern void	setbuf __P((FILE*, char*));
extern void	setlinebuf __P((FILE*));
extern void	setbuffer __P((FILE*, char*, int));
extern int	setvbuf __P((FILE*, char*, int __mode, size_t __size));
extern int	sprintf __P((char*, __const char* __format, ...));
extern int	sscanf __P((__const char* __string, __const char* __format, ...));
extern char*	tempnam __P((__const char *__dir, __const char *__pfx));
extern FILE*	tmpfile __P((void));
extern char*	tmpnam  __P((char *__s));
extern int	ungetc __P((int __c, FILE* __fp));
extern int	vfprintf __P((FILE *__fp, char __const *__fmt0, _G_va_list));
extern int	vfscanf __P((FILE *__fp, char __const *__fmt0, _G_va_list));
extern int	vprintf __P((char __const *__fmt, _G_va_list));
extern int	vscanf __P((char __const *__fmt, _G_va_list));
extern int	vsprintf __P((char* __string, __const char* __format, _G_va_list));
extern int	vsscanf __P((__const char* __string, __const char* __format, _G_va_list));

#if !defined(__STRICT_ANSI__) || defined(_POSIX_SOURCE)
extern FILE*	fdopen __P((int, __const char *));
extern int	fileno __P((FILE*));
extern int	pclose __P((FILE*));
extern FILE*	popen __P((__const char*, __const char*));
#endif

extern int __underflow __P((struct streambuf*));
extern int __overflow __P((struct streambuf*, int));

#if defined(__OPTIMIZE__) && !defined(NO_STDIO_MACRO)

#define getc(fp)		((fp)->_gptr >= (fp)->_egptr && \
				__underflow((struct streambuf*)(fp)) \
				== EOF ? EOF \
				: *(unsigned char*)(fp)->_gptr++)
#define getchar()		getc(stdin)
#define putc(c,fp)		(((fp)->_pptr >= (fp)->_epptr) ? \
				__overflow((struct streambuf*)(fp), \
				(unsigned char) (c)) \
				: (unsigned char)(*(fp)->_pptr++ = (c)))
#define putchar(c)		putc(c, stdout)
#define	clearerr(stream)	((stream)->_flags &= \
				~(STDIO_S_ERR_SEEN | STDIO_S_EOF_SEEN))
#define	feof(stream)		(((stream)->_flags & \
				STDIO_S_EOF_SEEN) ? EOF : 0)
#define	ferror(stream)		(((stream)->_flags & \
				STDIO_S_ERR_SEEN) != 0)

#endif	/* not Optimizing */

/* Print a message describing the meaning of the value of errno.  */
extern void perror __P ((__const char *__s));

#ifdef  __USE_BSD
extern int sys_nerr;
extern char *sys_errlist[];
#endif
#ifdef  __USE_GNU
extern int _sys_nerr;
extern char *_sys_errlist[];
#endif

#ifdef  __USE_MISC
/* Print a message describing the meaning of the given signal number. */
extern void psignal __P ((int __sig, __const char *__s));
#endif /* Non strict ANSI and not POSIX only.  */

extern char *__stdio_gen_tempname __P ((__const char *__dir,
		__const char *__pfx, int __dir_search,
		size_t * __lenptr));

__END_DECLS

#endif /*!_STDIO_H*/
