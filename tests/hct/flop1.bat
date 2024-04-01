echo off
cls
if "%2" == "" echo Syntax: FLOP src-drive dst-drive
if "%2" == "" echo To copy a file to and from floppy, verifying each
if "%2" == "" echo time.  It copies File1.src from src-drive to
if "%2" == "" echo dst-drive:\Tmpfile.one and then compares them.
if "%2" == "" echo Include ':' when specifying drives.
if "%2" == "" goto end

copy file1.src %1
copy %1file1.src %1tmpfile.one
:loop
copy %1file1.src %2tmpfile.one
compare %1tmpfile.one %2tmpfile.one
if errorlevel 1 goto error1
echo Compare of %1\Tmpfile.one and %2\Tmpfile.one OK!
copy %2tmpfile.one %1tmpfile.one
compare %1file1.src %1tmpfile.one
if errorlevel 1 goto error2
echo Compare of %1\File1.src and %1\Tmpfile.one OK!
goto loop

:error1
echo ERROR: Files do not compare after copy to floppy diskette.
goto end
:error2
echo ERROR: Files do not compare after copy from floppy diskette.

:end
