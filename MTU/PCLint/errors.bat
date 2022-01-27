@echo off
SETLOCAL EnableDelayedExpansion
rem example:
rem errors ge_kv2c_pjt ge_kv2c
rem to use PCLint plus
rem errors ge_kv2c_pjt ge_kv2c x
rem call linsetup
pushd r:
if exist "r:errors_%2.txt" del /q "r:errors_%2.txt"
if exist r:errors.txt del /q r:errors.txt
if exist r:files.txt del /q r:files.txt
if exist r:counts.txt del /q r:counts.txt
if exist "r:counts_%2.txt" del /q "r:counts_%2.txt"

set os=%2
set os=%os:~0,3%
echo %os%


rem Check parameter 2 to be like k22* or K22* ( e.g., K22_ge_i210+, K22_ge_kv2c )
if "%os%" == "k22" (
   set OS_DIR="K22_OS"
) else if "%os%" == "K22" (
   set OS_DIR="K22_OS"
) else (
   set OS_DIR="OS"
)

echo OSDIR - %OS_DIR%
rem Find the list of c modules in the IAR project file; place in %1.lnt
call r:pfiles.bat %1 %2

rem Collect include paths
call r:includes.bat %2 r:includes
sed -i -e "s/\"\"/\"/g" "V:\MTU\PCLint\includes.lnt"

rem run lint
rem echo "arg3" = "%3"
rem echo linting %2
rem if arg3 is not "" then use legacy PC-Lint
rem echo call lin -i"V:\MTU\%2\project" %1.lnt %3
if NOT [%1] == [] call lin -i"V:\MTU\%2\project" %1.lnt %3
rem if NOT [%1] == [] call lin -i"V:\MTU\%2\project" %1.lnt

rem In the lint file, count the occurrences of each file name in the project file followed by spaces and a number
echo Counting errors per source module for %2
rem for /f %%i in (r:%1.lnt) do grep -E "\W%%i\W+[0-9]+\s\{2}\(supplemental\)\@!" r:_lint_grep.txt | wc -l >> r:counts.txt
for /f %%i in (r:%1.lnt) do grep -E "\W%%i\W+[0-9]+" r:_lint_grep.txt | grep -icv supplemental >> r:counts.txt
rem for /f %%i in (r:%1.lnt) do grep -E "\W%%i\W+[0-9]+" r:_lint_grep.txt | wc -l >> r:counts.txt

echo renaming r:_lint_grep.txt "r:_lint_grep_%2.txt"
if exist "r:_lint_grep_%2.txt" del /q "_lint_grep_%2.txt"
ren r:_lint_grep.txt "_lint_grep_%2.txt"

rem combine the file list and counts into errors.txt - file name followed by error count; sorted by count (high to low)
paste r:%1.lnt r:counts.txt | sortunix -k2 -n -r | sed --binary -e "s/\r//g" > r:errors.txt
echo renaming r:errors.txt "r:errors_%2.txt"
if exist "errors_%2.txt" del /q "errors_%2.txt"
ren r:errors.txt "errors_%2.txt"

if exist r:files.txt del /q r:files.txt
if exist r:counts.txt del /q r:counts.txt
if exist r:sed*. del /q r:sed*.
popd
