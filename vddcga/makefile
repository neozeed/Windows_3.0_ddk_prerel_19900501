# sample make file for building vddcga virtual device for Windows/386

# comment this definition out with a "#", if building a non-debugging version
Debug=-DDEBUG

.asm.obj:
        masm5 -p -w2 -Mx $(Debug) -I..\include $*;

.asm.lst:
        masm5 -l -p -w2 -Mx $(Debug) -I..\include $*;

all:    vddcga.386

cgafont.obj cgafont.lst: cgafont.asm ..\include\debug.inc cga.inc \
	 ..\include\vmdacga.inc ..\include\vdd.inc ..\include\vmm.inc

cgainit.obj cgainit.lst: cgainit.asm ..\include\debug.inc ..\include\vdd.inc \
	 cga.inc ..\include\vmdacga.inc ..\include\vmm.inc

cgaint.obj cgaint.lst: cgaint.asm ..\include\debug.inc cga.inc \
	 ..\include\vmdacga.inc ..\include\vdd.inc ..\include\vmm.inc

cgamem.obj cgamem.lst: cgamem.asm ..\include\debug.inc cga.inc \
	 ..\include\vmdacga.inc ..\include\vdd.inc ..\include\vmm.inc

cgamsg.obj cgamsg.lst: cgamsg.asm ..\include\vmm.inc

cgaproc.obj cgaproc.lst: cgaproc.asm ..\include\debug.inc \
	 ..\include\vddgrb.inc ..\include\shell.inc ..\include\shellfsc.inc \
	 cga.inc ..\include\vmdacga.inc ..\include\vdd.inc ..\include\vmm.inc

cgasave.obj cgasave.lst: cgasave.asm ..\include\vdd.inc ..\include\debug.inc \
	 cga.inc ..\include\vmdacga.inc ..\include\vmm.inc

cgastate.obj cgastate.lst: cgastate.asm ..\include\debug.inc cga.inc \
	 ..\include\vmdacga.inc ..\include\vdd.inc ..\include\vmm.inc

cgasvc.obj cgasvc.lst: cgasvc.asm ..\include\debug.inc ..\include\shell.inc \
	 ..\include\shellfsc.inc cga.inc ..\include\vmdacga.inc \
	 ..\include\vdd.inc ..\include\vmm.inc

cgatio.obj cgatio.lst: cgatio.asm ..\include\debug.inc ..\include\vdd.inc \
	 cga.inc ..\include\vmdacga.inc ..\include\vmm.inc


OBJS =  cgafont.obj cgaint.obj cgamsg.obj cgasave.obj cgasvc.obj \
        cgainit.obj cgamem.obj cgaproc.obj cgastate.obj cgatio.obj 

vddcga.386: vddcga.def $(OBJS)
        link386 @vddcga.lnk
        addhdr vddcga.386
        mapsym32 vddcga
