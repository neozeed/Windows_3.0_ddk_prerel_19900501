cd ..\isg_test
command /c makelib
cd..\prnttest\frame
command /c makelib
cd..
erase *.obj
erase *.res
erase *.exe
erase *.lib
erase *.sym
make prnttest
