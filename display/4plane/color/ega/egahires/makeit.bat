@echo off
set t_inc=%masm%
cd ..\..\..
set masm=-DPUBDEFS -T -D?QUIET
make /S displays
if errorlevel 1 goto last
cd rc-med
make /S rc-med
cd ..\color
set masm=-DPUBDEFS -T -D?QUIET -D_NEW_COLOR_ADD
make /S color
if errorlevel 1 goto last
cd ega
set masm=-DPROTECTEDMODE -DPUBDEFS -T -D?QUIET -DDEBUG
make /S ega
if errorlevel 1 goto last
cd egahires
set masm=-DPROTECTEDMODE -DPUBDEFS -T -D?QUIET
make /S OPT="-D_EGA=1 -D_VGA=0" egahires
if NOT "%1" == "all" goto last
cd .\..\..\..
:last
set masm=%t_inc%
set t_inc=
echo on

