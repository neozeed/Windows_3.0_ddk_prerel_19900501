echo off
rem  NAME
rem	prep.bat
rem
rem  SYNOPSIS
rem	prep lang [option]
rem
rem	where:
rem
rem	lang	is the language you should extract (us,fr,gr,sp,it)
rem
rem	option	is the PVCS get option (i.e. -V1.03 will get version
rem		1.03 of the source code)
rem
rem  DESCRIPTION
rem	prep prepares your source directories for a subsequent build by
rem	extracting a read-only copy of your source from VCS logfiles.  Since
rem	some of these files may reside in localization directories, the master
rem	build process passes prep the language directory it is attempting to
rem	build.	If this is not appropriate for the directory you are prepping,
rem	then just change your batch file to never attempt to extract from
rem	language directories.
rem
rem	The option parameter provides a means for extracting source from a
rem	previously built rem version of Windows for the purpose of rebuilding.
rem
rem	The presence of a semaphore file tells prep not to extract this
rem	directory again.  When prep successfully extracts all the files, it
rem	creates this file for later reference.	It will be deleted by clean.bat
rem	when the master build is over.
rem
rem

rem	Just NOP for now
	goto exit

	if exist ~prp0000.tmp goto exit

	cd cgaherc\cga
	get %2 *.??^

	cd ..\hercules
	get %2 *.??^

	cd ..\multimod
	get %2 *.??^

	cd ..
	get %2 *.??^

	cd ..\ega\egamono
	get %2 *.??^

	cd ..\egacolor
	get %2 *.??^

	cd ..
	get %2 *.??^

	cd ..
	get %2 *.??^

	echo msw >> ~prp0000.tmp

:exit

