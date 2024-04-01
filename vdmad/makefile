# sample make file for building vdmad virtual device for Windows/386

# comment this definition out with a "#", if building a non-debugging version
Debug=-DDEBUG

.asm.obj:
        masm5 -p -w2 -Mx $(Debug) -I..\include $*;

.asm.lst:
        masm5 -l -p -w2 -Mx $(Debug) -I..\include $*;

all:    vdmad.386

dmaeisa.obj dmaeisa.lst: dmaeisa.asm ..\include\debug.inc dmasys.inc \
	 ..\include\vdmad.inc ..\include\vmm.inc

vdmad.obj vdmad.lst: vdmad.asm ..\include\sysinfo.inc ..\include\vkd.inc \
	 ..\include\vkdpaste.inc ..\include\debug.inc dmasys.inc \
	 ..\include\vdmad.inc ..\include\vpicd.inc ..\include\vmm.inc

vdmadapi.obj vdmadapi.lst: vdmadapi.asm dmasys.inc ..\include\vdmad.inc \
	 ..\include\debug.inc ..\include\vmm.inc

vdmadio.obj vdmadio.lst: vdmadio.asm ..\include\debug.inc dmasys.inc \
	 ..\include\vdmad.inc ..\include\vmm.inc

vdmadmsg.obj vdmadmsg.lst: vdmadmsg.asm ..\include\vmm.inc

vdmadps2.obj vdmadps2.lst: vdmadps2.asm ..\include\debug.inc dmasys.inc \
	 ..\include\vdmad.inc ..\include\vmm.inc


OBJS =  dmaeisa.obj vdmad.obj vdmadapi.obj vdmadio.obj vdmadmsg.obj vdmadps2.obj 

vdmad.386: vdmad.def $(OBJS)
        link386 @vdmad.lnk
        addhdr vdmad.386
        mapsym32 vdmad
