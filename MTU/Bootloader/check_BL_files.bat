:: Check for filename.c/h vs filename_BL.c/h differences
@echo off
echo Check for BL File Changes, Version 1.1, 8/30/22

:: set Winmerge install location
set WINMERGE_PATH="C:\Program Files\WinMerge\WinMergeU.exe"

:: set Beyond Compare install location
set BEYOND_COMPARE_PATH="C:\Program Files\Beyond Compare 4\BComp.com"

if exist %BEYOND_COMPARE_PATH% (
	set MERGE=%BEYOND_COMPARE_PATH%
) else (
	if exist %WINMERGE_PATH% (
		set MERGE=%WINMERGE_PATH%
	) else (
		echo Error: requires either %BEYOND_COMPARE_PATH% or %WINMERGE_PATH% - exiting
		exit /b 0
	)
)
echo Using %MERGE% for comparison

:: make the comparison folders
set COMPARE_FOLDER=%cd%\compare
mkdir %COMPARE_FOLDER%
mkdir %COMPARE_FOLDER%\bl
mkdir %COMPARE_FOLDER%\base
set BL_COMPARE_FOLDER=%cd%\compare\bl
set BASE_COMPARE_FOLDER=%cd%\compare\base

:: allow delayed expansion in the for loop
setlocal enabledelayedexpansion

:: move to root folder
cd ..\..
echo Searching from: %cd%
echo Creating compare folders at %COMPARE_FOLDER%...

:: look for _BL files and base files to add to compare folder
for /r %%a in (*_BL*.c *_BL*.h) do (
	set BL_FILE=%%a
	set BL_FILENAME=%%~nxa
	set BASE_FILENAME=!BL_FILENAME:_BL=!
	set BASE_FILE=!BL_FILE:_BL=!
	echo copying !BL_FILE! to compare\bl
	@copy !BL_FILE! !BL_COMPARE_FOLDER!\!BASE_FILENAME! > NUL
	if exist !BASE_FILE! (
		echo copying !BASE_FILENAME! to compare\base
		@copy !BASE_FILE! !BASE_COMPARE_FOLDER! > NUL
	) else (
		echo no base file for !BL_FILENAME!
	)
)
setlocal disabledelayedexpansion
cd MTU\Bootloader

:: start winmerge
%MERGE% %BL_COMPARE_FOLDER% %BASE_COMPARE_FOLDER%

echo Removing compare folders...
rmdir /s /q %COMPARE_FOLDER%

echo Done!