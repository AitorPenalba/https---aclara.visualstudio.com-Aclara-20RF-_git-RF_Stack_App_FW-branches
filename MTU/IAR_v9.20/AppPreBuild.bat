@echo off
rem Delete version.o to force it to be compiled with every change.  This will cause the build Date/Time to be updated.
rem Removed since it is not working with 9.20 - deletes, but compiles every other time: if exist "%~1\version.o" (del "%~1\version.o")
rem in IAR Build Actions, Pre-build command line:cmd /c if exist "$OBJ_DIR$\version.o" (del "$OBJ_DIR$\version.o")
