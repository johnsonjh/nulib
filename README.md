# NuLib v3.24

## History

### NuLib v3.24 (January 1993)

- Improved MS-DOS filename fixing (now enforce "8 chars '.' 3 chars")
- Fixed bug in LZW decompressor

### NuLib v3.23 (December 1992)

- Minor bug fixes for the MS-DOS version (Dale G. Shields)

### NuLib v3.22 (September 1992)

- Added "compress file as if it were a disk" code (provided by somebody
  whose name I just lost). Seems to work, but it's not 100% tested.
- Faster compression
- Updated filetype abbreviations

### NuLib v3.21

- Minor fixes

### NuLib v3.2 (April 1992):

- Lots of minor bug fixes. (I was getting lost in the version numbers so
  I just upped it to 3.2). Of importance are the bug fixes to the LZW-II
  decoder and EFT (Expanded Fundamental Types) awareness.

### NuLib v3.14 (November 1991)

- Finished fixing NeXT probs. Really. (Chris Osborn w/push from Bruce Kahn)
- Fixed some minor bugs (Larry W. Virden)

### NuLib v3.13

- Repaired XENIX directory stuff in "nuadd.c" (Morgan Davis)

### NuLib v3.12

- Fixed problem with table clears on non-LZW blocks (Frank Petroski)

### NuLib v3.11

- Added some stuff to make the NeXT happy (Chris Osborn)

### NuLib v3.1 (October 1991)

- GS/ShrinkIt LZW-II uncompression (finally!)
- All ShrinkIt compression/uncompression routines are about 15% faster
- Better compatibility with System V, especially the AT&T 3B2

### Nulib v3.03 (February 1991)

- Fixed XENIX problems with includes and libs (Ron Higgins).
- Fixed bug in directory expansion (Larry W. Virden).

### NuLib v3.02

- Silenced screaming about bad dates (Larry W. Virden).
- Fixed glitches in nulib.lnk and nulib.mak (Bruce Kahn).

### NuLib v3.01

- Fixed non-compression bug in ShrinkIt LZW (Scott Blackman).

### NuLib v3.0 (September 1990)

- ShrinkIt LZW compression
- UNIX 12-bit and 16-bit compression
- New archive listing format (similar to ARC and ZOO)

### Notes

```text
Things work much better if you read the documentation file (now available
on the archive site... sorry I didn't put it up sooner).  If you want to
use UNIX compress to store files, READ THE DOCUMENTATION FIRST.  Not only
does it tell you how to do it, but it has some warnings about compatibility.

To compile this on a UNIX-type system, edit the file "nudefs.h".  Several
systems are predefined; the default is a BSD UNIX system.  If you want to run
this on something other than BSD, comment out the #define statements (using
"/*" and "*/"), and uncomment the appropriate statements (several systems are
defined... if yours is an AT&T System V system, try the defines for Amdahl
UTS).  Then, type "make" to execute the Makefile.  If all goes well, you will
be left with an executable file called "nulib".  If all does not go well,
double check "nudefs.h".  You may need to deal with the HAS_EFT stuff,
especially if your compiler complains that "mode_t" or "off_t" are undefined.
Send some mail to me if you can't get it to work at all.

To make the MS-DOS version, use the nulib.mak and nulib.lnk files with MS C
(supplied by Bruce Kahn).  To make the APW C version (NOT Orca/C), put all
the files in one directory, and make a subdirectory called "OBJ".  Put the
"linked.scr" and "linker.scr" files in OBJ, and then "make.apw".  This will
compile all the files.  When it finishes, change to OBJ and "alink
linked.scr" (standard linker) or "alink linker.scr" (ZapLink).  The makefile
used to do this automatically, but with only 1.25MB of RAM you have to exit
APW to purge all the memory used by the compiler, and and then restart APW
and run the linker.

In both cases, BE SURE to modify "nudefs.h" first.  If you don't, the
compilation will halt in "numain.c" on a line which reminds you to change
"nudefs.h" (I tended to forget about this).  Also, under APW make sure all
the file types are set to SRC, and the auxtype to CC.  These are changed with
the commands "filetype" and "change", respectively.

Please send bug reports, ideas, or gripes to fadden@uts.amdahl.com, or to
one of the addresses mentioned in the documentation or under the "-hw"
option.
```

### Known Bugs

- Under some systems, using UNIX compress on a file which does not compress
  will cause the archive's EOF to be larger than it should be. This slack
  space will be used up if you add more files to the archive (with NuLib
  anyway; no guarantees about ShrinkIt or GS/ShrinkIt, but there's no reason
  why they shouldn't work).

- Just to make things clear: if the file being compressed doesn't get any
  smaller, the compression halts and the file is simply stored uncompressed.
  Because of the way compress works, on some systems the space that would
  have been occupied by more compressed data is left attached to the file.
  The ShrinkIt compression does not suffer from this problem. If you add
  more stuff to the file, NuLib will fill the slack space first, NOT just
  append to the end of the file).

### Porting Notes

- Linux Port

  - This was pretty easy. The only thing linux complained about was an
    incompatibility in the rename() function so I commented it out of stdio.h and
    defined NO_RENAME in nudefs.h and added a linux def there.
    - Mike Neuliep \<wires@gnu.ai.mit.edu\>

- Modern Linux Port
  - Added somewhat recent \_G_config.h, commented a few lines, and changed
    Makefile to use -fcommon. More work is likely needed, but has successfully
    extracted Binary \]\[ and NuLib archives.
  - Added somewhat recent \_G_config.h, commented a few lines, and changed Makefile to use -fcommon. More work is likely needed, but has successfully extracted Binary \]\[ and NuLib archives.
    - Jeffrey H. Johnson \<trnsz@pobox.com\>

### Original Author

- \<fadden@uts.amdahl.com\>
