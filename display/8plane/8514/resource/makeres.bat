if exist 8514.res erase 8514.res
if exist config.bin erase config.bin
if exist fonts.bin erase fonts.bin
make res8514
if ERRORLEVEL 1 Goto EndLabel
del fonts.obj
del fonts.exe
del config.obj
del config.exe
:EndLabel
