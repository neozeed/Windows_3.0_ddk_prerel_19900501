This directory contains the MT utility for converting Windows
2.0 keyboard translation tables into Windows 3.00 tables.

The MT makefile provides an example:  The various 2.0 tables for
Icelandic are first assembled and linked, and run through exe2bin,
to produce binary files (*.WK2), 1 for each keyboard type.

MT.EXE then reads these and produces the 3.0 keyboard translation
table, KBDIC.ASM. This may be hand-edited, if necessary.

