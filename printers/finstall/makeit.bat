echo off
echo Making FINSTALL.DLL Font Installer
cd src
make finstall
cd ..\rc
make finstall
cd ..
make finstall
