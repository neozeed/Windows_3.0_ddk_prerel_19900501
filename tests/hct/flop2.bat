echo off
cls
if "%2" == "" echo Syntax: FLOP src-drive dst-drive
if "%2" == "" echo To copy a file to and from floppy, verifying each
if "%2" == "" echo time.  It copies File2.src from src-drive to
if "%2" == "" echo dst-drive:\Tmpfile.two and then compares them.
if "%2" == "" echo Include ':' when specifying drives.
if "%2" == "" goto end

copy file2.src %1
copy %1file2.src %1tmpfile.two
:loop
copy %1file2.src %2tmpfile.two
compare %1tmpfile.two %2tmpfile.two
if errorlevel 1 goto error1
echo Compare of %1\Tmpfile.two and %2\Tmpfile.two OK!
copy %2tmpfile.two %1tmpfile.two
compare %1file2.src %1tmpfile.two
if errorlevel 1 goto error2
echo Compare of %1\File2.src and %1\Tmpfile.two OK!
goto loop

:error1
echo ERROR: Files do not compare after copy to floppy diskette.
goto end
:error2
echo ERROR: Files do not compare after copy from floppy diskette.

:end
