@echo off
rem example:
rem includes ge_kv2c includes
setlocal EnableDelayedExpansion
if exist %2.lnt del %2.lnt
rem 1st sed script "/CCIncludePath2/,/option/{/CCIncludePath2/b;/option/b;p}" : Keep only lines between CCIncludePath2 and option
rem There is a set of these per configuration (Debug, Release, etc.), so sort and uniq to get only one copy of each
rem 2nd sed script "s/<\/*state>//g" removes "state" tags e.g. <state>main.c</state>
rem 3rd sed script "s/^\s*\$PROJ_DIR\$\(\/\.\.\)\{2\}/-i\"V:\" $PROJ_DIR$\..\.. becomes V:\
rem 4th sed script "s/^\s*\$PROJ_DIR\$\(\/\.\.\)\{1\}/-i\"V:\MTU/" $PROJ_DIR$\.. becomes V:\MTU
rem 5th sed script "s/$/\"/p" appends " to the end of each line
set proj=%1
set proj=%proj:~0,4%
if /I "%proj%"=="boot" (
   echo +d"__BOOTLOADER">%2.lnt
   echo -i"V:\MTU\BootLoader">>%2.lnt
)
rem sed -n "/CCIncludePath2/,/option/{/CCIncludePath2/b;/option/b;p}" V:\MTU\iar\%1.ewp | sortunix -d | uniq >> %2.lnt
rem sed %2.lnt -n -i -e "s/<\/*state>//g" -e s/^\s*\$PROJ_DIR\$\([/\\]\.\.\)\{2\}/-i\"V:/ -e "s/^\s*\$PROJ_DIR\$\([/\\]\.\.\)\{1\}/-i\"V:\\MTU/" -e "/TOOLKIT_DIR/d" -e "/^\s*$/d" -e "s/$/\"/p"


rem Revised! Since the .ewp files often have Debug AND Release configurations, the previous method got BOTH sets of include directories.This causes
rem warning messages to be emitted by Lint.
rem The revised method deletes all lines from the beginning to the <configuration><Debug> pair. It then deletes all lines from there to the
rem CCIncludePath2 line. It then deletes all lines after the closing </option> line. This eliminates the Release configuration whether it comes 1st or
rem 2nd. It then proceeds with the same substitutions and sorting as before.
sed -e "{N;0,/<configuration>.*<name>Debug/d}" V:\MTU\iar\%1.ewp | sed -e "{0,/CCIncludePath2/d}" | sed -e "{/<\/option>/,$d}" | sortunix -d | uniq >> %2.lnt
sed %2.lnt -n -i -e "s/<\/*state>//g" -e s/^\s*\$PROJ_DIR\$\([/\\]\.\.\)\{2\}/-i\"V:/ -e "s/^\s*\$PROJ_DIR\$\([/\\]\.\.\)\{1\}/-i\"V:\\MTU/" -e "/TOOLKIT_DIR/d" -e "/^\s*$/d" -e "s/$/\"/p"
