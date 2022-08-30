:: Check for differences between the RASC created FSP folders and the locked-in bootloader folders
@echo off
echo Check for RA6E1 Bootloader FSP File Changes, Version 1.1, 8/30/22

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

:: start visual merge
%MERGE% /r %cd% %cd%\IAR

echo Done!