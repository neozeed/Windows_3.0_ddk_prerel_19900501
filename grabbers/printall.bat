cd cgaherc
del c:\tmp <c:\lib\yes.in >nul
fnd . /cp c:\tmp
del c:\tmp\*.grb
ds en c:\tmp
command /e:996 /c p3000 *.* c:\tmp

cd ..\ega
del c:\tmp <c:\lib\yes.in >nul
fnd . /cp c:\tmp
del c:\tmp\*.grb
ds en c:\tmp
command /e:996 /c p3000 *.* c:\tmp

cd ..
del c:\tmp <c:\lib\yes.in >nul
copy . c:\tmp
ds en c:\tmp
command /e:996 /c p3000 *.* c:\tmp

del c:\tmp <c:\lib\yes.in >nul
