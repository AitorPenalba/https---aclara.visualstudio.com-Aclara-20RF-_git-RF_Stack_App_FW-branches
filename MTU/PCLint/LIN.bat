@echo off
rem L must be mapped to the path where PCLint is installed
rem I must be mapped to the project directory
rem R must be mapped to the project's lint folder
rem linsetup.bat performs these mappings
rem call linsetup
@echo on
pushd v:
"L:\Lint-nt" -i"R:" -i"L:"  -i"L:\lnt" -width(0) +v  R:\std.lnt  -os(R:\_LINT.TXT) %1 %2 %3 %4 %5 %6 %7 %8 %9
@echo off
echo  output placed in R:\_LINT.TXT
rem grep "Warning\|Error\|Info" R:\_LINT.TXT > R:\_LINT.TMP
"C:\Program Files (x86)\GnuWin32\bin\grep" -v "cited in prior" R:\_LINT.TXT > R:\_LINT_GREP.TXT
if exist R:_LINT.TMP del R:\_LINT.TMP
echo Filtered output placed in R:\_LINT_GREP.TXT
popd