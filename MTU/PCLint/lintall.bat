@echo off
setlocal EnableDelayedExpansion
for %%i in (v:\mtu\iar\*.ewp) do (
   if exist V:\MTU\PCLint\includes.lnt del /q V:\MTU\PCLint\includes.lnt
   set "proj=%%~ni"
rem    echo %proj%
   r:\errors.bat %proj%_pjt %%~ni %1
)
