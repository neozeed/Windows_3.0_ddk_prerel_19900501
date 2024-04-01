echo off
REM this avoids recursive execution of MAKE.EXE...
echo Make HPPCL.RC
cd src
echo making .OBJ files
make pclsrc
cd ..\rc
echo making .RES file
make pclrc
cd ..
echo linking
make hppcl
