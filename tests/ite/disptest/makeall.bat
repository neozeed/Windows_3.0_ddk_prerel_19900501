cd ..\isg_test
command /c makelib
cd..\disptest
erase *.obj
erase *.res
erase *.exe
erase *.lib
erase *.sym
make DispTest
