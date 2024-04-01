cd ..\..\..
make /S OPT="-DPUBDEFS -T -D?QUIET" displays
cd rc-low
make /S rc-low
cd ..\bw
make /S OPT="-DPUBDEFS -T -D?QUIET" blkwhite
cd cgaherc\cga
make /S OPT="-DPUBDEFS -T -D?QUIET -DDEBUG -DIBM_CGA" cga
if NOT "%1" == "all" goto last
cd .\..\..\..
:last
echo on

