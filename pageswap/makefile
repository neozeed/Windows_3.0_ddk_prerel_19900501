# sample make file for building pageswap virtual device for Windows/386

# comment this definition out with a "#", if building a non-debugging version
Debug=-DDEBUG

.asm.obj:
        masm5 -p -w2 -Mx $(Debug) -I..\include $*;

.asm.lst:
        masm5 -l -p -w2 -Mx $(Debug) -I..\include $*;

all:    pageswap.386

pageswap.obj pageswap.lst: pageswap.asm spoem.inc spart.inc \
	 ..\include\pageswap.inc ..\include\pdb.inc ..\include\smartdrv.inc \
	 ..\include\vkd.inc ..\include\vkdpaste.inc ..\include\vhd.inc \
	 ..\include\shell.inc ..\include\shellfsc.inc ..\include\v86mmgr.inc \
	 ..\include\dosmgr.inc ..\include\debug.inc ..\include\vmm.inc

pstext.obj pstext.lst: pstext.asm ..\include\vmm.inc


OBJS =  pageswap.obj pstext.obj 

pageswap.386: pageswap.def $(OBJS)
        link386 @pageswap.lnk
        addhdr pageswap.386
        mapsym32 pageswap
