@echo off
set t_inc=%masm%
set masm= -DPUBDEFS -T -D?QUIET
cd ..\..\..
make /S displays
if errorlevel 1 goto last
cd rc-high
make /S rc-high
cd ..\color
set masm= -DPUBDEFS -T -D?QUIET -D_NEW_COLOR_ADD
make /S OPT="-D_VGA=1" color
if errorlevel 1 goto last
cd ega
set masm= -DPROTECTEDMODE -DPUBDEFS -T -D?QUIET -DDEBUG
make /S ega
if errorlevel 1 goto last
cd vga
set masm= -DPROTECTEDMODE -DPUBDEFS -T -D?QUIET
make /S OPT="-D_VGA=1 -D_EGA=0" vga
if NOT "%1" == "all" goto last
cd .\..\..\..
:last
set masm=%t_inc%
set t_inc=
echo on

