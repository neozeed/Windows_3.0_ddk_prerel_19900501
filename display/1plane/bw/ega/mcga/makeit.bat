cd ..\..\..
make /S OPT="-DPUBDEFS -T -D?QUIET" displays
cd rc-high
make /S rc-high
cd ..\bw
make /S OPT="-DPUBDEFS -T -D?QUIET" blkwhite
cd ega
make /S OPT="-DPUBDEFS -T -D?QUIET -DDEBUG" ega
if errorlevel 1 goto last
cd mcga
make /S OPT="-DPUBDEFS -DVGA_MONO -T -D?QUIET" icarus
if NOT "%1" == "all" goto last
cd .\..\..\..
:last
echo on

