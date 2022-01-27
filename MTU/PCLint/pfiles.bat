@echo off
rem example:
rem pfiles ge_kv2c_pjt ge_kv2c
rem echo pfiles.bat arg 1 = %1
grep "\.c" V:\mtu\iar\%2.ewp > files.txt 
rem remove xml tags surrounding C file names and leading spaces and \r from line endings
sed -e "s/<\/*name>//g" -e "s/^.*\\//" files.txt | sortunix | uniq | sed --binary -e "s/\r//g" > %1.lnt
