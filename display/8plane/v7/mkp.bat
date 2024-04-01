if exist res\96x96\rc-high.res goto driver
cd res\96x96
make rc-high
cd ..\..
:driver
rem make %1 opt="-DPUBDEFS -DPROTECTEDMODE -DVRAM480" drvs="640x480" VRAM256" >\errs
make %1 opt="-DPUBDEFS -DPROTECTEDMODE -DVRAM480" VRAM256" >\errs
