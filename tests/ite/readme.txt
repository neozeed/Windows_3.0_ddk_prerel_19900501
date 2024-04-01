ITE - Integrated Test Environment

     This is an environment of common libraries and executables for testing
     Windows Device Drivers.  The development structure is organized in a
     very strict order since the applications are dependant on the location
     of IMPORT LIBRARIES as well as DYNALINK EXECUTABLES.

     Those directories beginning with "ITE_" signify DynaLink directories.
     All common libraries will begin this these 4 characters.

     Batch files are provided at every directory level to assists in the
     building of an application, or the entire project if need be.  It is
     HIGHLY recommended that these batch files be run, rather than the
     standard MAKE utility.

          MAKEAPP.BAT/MAKELIB.BAT - Will build an APPLICATION or LIBRARY and
                                    copy all appropriate files to the \BIN,
                                    \LIB, and \DEF directories.

          MAKEALL - This batch file will build an entire APPLICATION or
                    LIBRARY and copy all appropriate files to the \BIN, \LIB,
                    and \DEF directories.  This differs from the MAKEAPP or
                    MAKELIB batch files in that it removes all ".OBJ" prior
                    to building.

          BUILDITE - This batch file will build the entire PROJECT and copy
                     the appropriate files to their corresponding
                     directories.

     Project Directories Descriptions.

     \BIN - This directory holds all built executables and libraries.  It is
            this directory that the applications should be run.

     \LIB - Temporary directory for holding all ".LIB" files generated from
            the building of the DynaLink libraries.

     \DEF - This holds the definition files for temporary storage for the
            generation of the import library ITE.LIB.

     \INC - Common include files.

     \PRNTTEST       - Printer Driver Test application sources.
     \PRNTTEST\FRAME - Printer Driver Test NEWFRAME library sources.
     \PRNTTEST\BAND  - Printer Driver Test BAND library sources.

     \DISPTEST - Display Driver Test application sources.
