:: Check for filename.c/h vs filename_BL.c/h differences
:: Version 1.0
@echo off
echo Check for BL File Changes, Version 1.0, 8/3/22

:: Add winmerge path here
set WINMERGE="C:\Program Files\WinMerge\WinMergeU.exe"

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

::for /f "tokens=*" %%a in ('dir /b *_BL.c') do echo %%a
::for /f "tokens=*" %%a in ('dir /b *_BL*') do (
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
%WINMERGE% %BL_COMPARE_FOLDER% %BASE_COMPARE_FOLDER%

echo Removing compare folders...
rmdir /s /q %COMPARE_FOLDER%

echo Done!