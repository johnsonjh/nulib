 # NuLib v3.02 makefile for Microsoft C
 #===================================================================
 #
 #  Standard command line definitions
 #
 #  Note /DIAPX286 can also be used, but it is not a real indicator
 #  of the system hardware
 #
 #  Leaving out the /W3 will reduce the flood of warning messages.
 #
 #===================================================================
 
 #cp=cl /c /AL /W3 /Ot /Zpei /DIAPX386
 cp=cl /c /AL /W3 /Os /Zpei /DIAPX386
 
 #===================================================================
 #
 #  Default inference rules
 #
 #===================================================================
 
 .c.obj:
    $(cp) $*.c
 
 #===================================================================
 #
 #  Dependencies
 #
 #===================================================================
 
 nuadd.obj: nuadd.c nudefs.h nuread.h nuview.h nuadd.h nupak.h nuetc.h
 
 nublu.obj: nublu.c nudefs.h nuview.h nuadd.h nupak.h nuetc.h
 
 nuetc.obj: nuetc.c nudefs.h apwerr.h nuetc.h
 
 nuext.obj: nuext.c nudefs.h nuread.h nuext.h nupak.h nuetc.h
 
 numain.obj: numain.c nudefs.h nuread.h nuview.h nuadd.h nuext.h nupdel.h \
       nublu.h nupak.h nuetc.h
 
 nupak.obj: nupak.c nudefs.h nupak.h nuetc.h nucomp.h nucompfn.h
 
 nupdel.obj: nupdel.c nudefs.h nuread.h nuadd.h nupak.h nupdel.h nuetc.h
 
 nuread.obj: nuread.c nudefs.h crc.h nuread.h nupak.h nuetc.h
 
 nushk.obj: nushk.c nudefs.h nuread.h nupak.h nuetc.h
 
 nusq.obj: nusq.c nudefs.h nuetc.h
 
 nuview.obj: nuview.c

nucomp.obj: nucomp.c nudefs.h nucomp.h nucompfn.h
 
 NULIB.exe: nuadd.obj nublu.obj nuetc.obj nuext.obj numain.obj nupak.obj  \
        nupdel.obj nuread.obj nushk.obj nusq.obj nuview.obj nucomp.obj
        link @NuLib.lnk
 
