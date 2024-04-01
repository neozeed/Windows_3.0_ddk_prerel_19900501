echo off
rem  NAME
rem	clean.bat
rem
rem  SYNOPSIS
rem	clean [lang option]
rem
rem	where:
rem
rem	lang	is the language directory you will update with the VCS
rem		version command
rem
rem	option	is the actual VCS version command
rem
rem  DESCRIPTION
rem	This routine cleans all source, intermediate, resultant, prep and error
rem	files which reside in and below this branch of the directory.
rem	It optionally will use the VCS program to insert version numbers in the
rem	existing log files.  Usually only the master build process will call
rem	clean with these options.  The version number installed can be used
rem	with subsequent prep calls to rebuild this version of Windows if
rem	necessary.
rem
rem
echo off

:cga
	cd cgaherc\cga
	for %%f in (obj exe) do del *.%%f
	if "%2" == "" goto hercules
	vcs %2 *.??^

:hercules
	cd ..\hercules
	for %%f in (obj exe) do del *.%%f
	if "%2" == "" goto multimod
	vcs %2 *.??^

:multimod
	cd ..\multimod
	for %%f in (obj exe) do del *.%%f
	if "%2" == "" goto cgaherc
	vcs %2 *.??^

:cgaherc
	cd ..
	for %%f in (obj %%%%%%) do del *.%%f
	if "%2" == "" goto egamono
	vcs %2 *.??^

:egamono
	cd ..\ega\egamono
	for %%f in (obj exe) do del *.%%f
	if "%2" == "" goto egacolor
	vcs %2 *.??^

:egacolor
	cd ..\egacolor
	for %%f in (obj exe) do del *.%%f
	if "%2" == "" goto ega
	vcs %2 *.??^

:ega
	cd ..
	for %%f in (obj %%%%%%) do del *.%%f
	if "%2" == "" goto grabbers
	vcs %2 *.??^

:grabbers
	cd ..
	del *.obj
	if "%2" == "" goto exit
	vcs %2 *.??^

:exit
	del    ~prp0000.tmp
	del ..\~err0000.tmp

