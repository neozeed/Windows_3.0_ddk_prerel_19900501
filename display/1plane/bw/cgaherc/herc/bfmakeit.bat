cd ..\..\..
make /S OPT="-DBIGFONT -DPUBDEFS -T -D?QUIET" bfdisp
cd rc-med
make /S rc-med
cd ..\bw
make /S OPT="-DPUBDEFS -T -D?QUIET" blkwhite
cd cgaherc
make /S OPT="-DBIGFONT -DPUBDEFS -T -D?QUIET -DDEBUG -DHERCULES" cgaherc
if errorlevel 1 goto last
cd herc
make /S OPT="-DBIGFONT -DPUBDEFS -T -D?QUIET -DHERCULES" bfherc
if NOT "%1" == "all" goto last
cd .\..\..\..
:last
echo on

