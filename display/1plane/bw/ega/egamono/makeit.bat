cd ..\..\..
make /S OPT="-DPUBDEFS -T -D?QUIET" displays
cd rc-med
make /S rc-med
cd ..\bw
make /S OPT="-DPUBDEFS -T -D?QUIET" blkwhite
cd ega
make /S OPT="-DPUBDEFS -T -D?QUIET -DDEBUG" ega
if errorlevel 1 goto last
cd egamono
make /S OPT="-DPUBDEFS -DEGA_MONO -T -D?QUIET" egamono
if NOT "%1" == "all" goto last
cd .\..\..\..
:last
echo on

