cd ..\..\..
make /S OPT="-DPUBDEFS -T -D?QUIET" displays
cd rc-high
make /S rc-high
cd ..\bw
make /S OPT="-DPUBDEFS -T -D?QUIET" blkwhite
cd cgaherc
make /S OPT="-DPUBDEFS -T -D?QUIET -DDEBUG -DHERCULES" cgaherc
if errorlevel 1 goto last
cd compaq
make /S OPT="-DPUBDEFS -T -D?QUIET -DHERCULES" compaq
if NOT "%1" == "all" goto last
cd .\..\..\..
:last
echo on
