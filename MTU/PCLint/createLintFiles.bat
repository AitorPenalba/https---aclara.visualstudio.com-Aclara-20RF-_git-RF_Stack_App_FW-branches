echo off
if exist V:\MTU\PCLint\files.txt del /q V:\MTU\PCLint\files.txt
if exist V:\MTU\PCLint\includes.lnt del /q V:\MTU\PCLint\includes.lnt

set os=%2
set os=%os:~0,3%
rem echo %os%


rem Check parameter 2 to be like k22* or K22* ( e.g., K22_ge_i210+, K22_ge_kv2c )
if "%os%" == "k22" (
   set OS_DIR="K22_OS"
) else if "%os%" == "K22" (
   set OS_DIR="K22_OS"
) else (
   set OS_DIR="OS"
)

rem echo OSDIR - %OS_DIR%
rem Find the list of c modules in the IAR project file; place in %1.lnt
echo build files for project %2; output to %1
call V:\MTU\PCLint\pfiles.bat %1 %2

rem Collect include paths
call V:\MTU\PCLint\includes.bat %2 includes
echo Done!
