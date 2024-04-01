:cga
	cd cgaherc\cga
	make cga
	if NOT errorlevel 1 goto hercules
	echo gen: cga grabber build error >> ..\..\..\~err0000.tmp

:hercules
	cd ..\hercules
	make hercules
	if NOT errorlevel 1 goto multimod
	echo gen: hercules grabber build error >> ..\..\..\~err0000.tmp

:egamono
	cd ..\..\ega\egamono
	make egamono
	if NOT errorlevel 1 goto egacolor
	echo gen: egamono grabber build error >> ..\..\..\~err0000.tmp

:egacolor
	cd ..\egacolor
	make egacolor
	if NOT errorlevel 1 goto exit
	echo gen: egacolor grabber build error >> ..\..\..\~err0000.tmp

:exit
	cd ..\..


