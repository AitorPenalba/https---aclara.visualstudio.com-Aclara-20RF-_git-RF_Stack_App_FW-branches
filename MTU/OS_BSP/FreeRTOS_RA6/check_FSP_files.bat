:: Check for differences between the RASC created FSP folders and the locked-in bootloader folders
:: Version 1.0
@echo off
echo Check for RA6E1 Bootloader FSP File Changes, Version 1.0, 8/17/22

:: Add winmerge path here
set WINMERGE="C:\Program Files\WinMerge\WinMergeU.exe"

:: start winmerge
%WINMERGE% /r %cd% %cd%\IAR

echo Done!