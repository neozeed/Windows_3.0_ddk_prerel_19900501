erase *.res
erase *.lib
erase *.sym
erase *.dll
erase ..\bin\isg_test.dll
make  Version=$(VER3) isg_test
copy  isg_test.dll ..\bin
copy  isg_test.lib ..\lib
copy  isg_test.h   ..\inc
