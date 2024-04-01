echo off
rem  NAME
rem	modules.bat
rem
rem  SYNOPSIS
rem	modules dirname
rem
rem	where:
rem
rem	dirname is the directory where the master build process wants the
rem		built modules to be put
rem
rem  DESCRIPTION
rem	This routine copies all built modules in and below this branch of the
rem	directory structure individually to the passed directory name
rem
rem
echo off

	copy cgaherc\cga\cga.gr2                %1
	copy cgaherc\hercules\hercules.gr2      %1
	copy ega\egamono\egamono.gr2            %1
	copy ega\egacolor\egacolor.gr2          %1
	copy ega\sv400a\sv400a.gr2              %1
        copy vga\vgamono\vgamono.gr2            %1
        copy vga\vgacolor\vgacolor.gr2          %1
        copy olivetti\oligrab\oligrab.gr2       %1

:exit

