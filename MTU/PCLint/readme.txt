The batch files located here require several GNU utilities: grep, sed, sort (sortunix*), paste, wc, uniq. They are
available for free from various sites, depending on the utility needed:
http://ftp.gnu.org/gnu/coreutils/
http://gnuwin32.sourceforge.net/packages.html

Copies of the installers are here: S:\Product Development Shared\Firmware\Tools\Unix command line tools

********************************************************************************************************
NOTE:
Rename sort.exe to sortunix.exe to prevent invoking the Windows version of sort which is not compatible.
********************************************************************************************************

errors.bat takes two  arguments
   Arg 1 - file to lint - typcially this is a .lnt file with a list of .c files to analyze.
   Arg 2 - passed to pfiles.bat

invokes pfiles.bat with arg 2
invokes includes.bat with arg 2

   for each file name listed in Samwise_pjt.lnt, searches the _lint_grep.txt file and counts the number of times that
   file is present (followed by blanks and a number). The totals are placed in counts.txt
   The counts are combined with the file names (paste.exe) and sorted by the counts, in reverse order.
   This results in a text file with two columns: filename and count. Helpful in figuring out which files have the most
   lint errors

pfiles.bat takes an optional argument
   Arg 1 - uses this as the basename to find IAR project to search (e.g., K22_GE_I210+ causes K22_GE_I210+.ewp to be
         used)
   extracts the .c entries from the chosen .ewp file, sorts them and puts them in Samwise_pjt.lnt

includes.bat
   extracts the include paths from the %1.ewp file and creates includes.lnt. includes.lnt is "included" in Samwise.lnt

examples:
errors Samwise_pjt ge_i210+
   finds the .c files in the ge_i210+ project (ge_i210+.ewp) and puts them in Samwise_pjt.lnt. 
   finds the include directories in the ge_i210+ project (ge_i210+.ewp) and puts them in includes.lnt. 
   creates a sorted list (errors_ge_i210+.txt) with the filenames and lint error count, and an error report file
   (_lint_grep_ge_i210+.txt).

Usage help notes:
#########################################
Things you need to do to run Lint:  
-check out and edit MyPath.lnt to match your path to SRFN top level directory
 Update 7/13/2017: MyPath.lnt now also sets the path to your compiler installation directory. Modify as necessary. There
 should be no need to check this file back in, but if you do, be sure to check in with settings as needed for the
 automated test system. 
-Need to check out Samwise_pjt.lnt, it is regenerated during this process. If it does change, be sure to check it back
 in so the automated test system has the correct list of .c files to use in its lint operation.
-Modify linsetup.bat to match your TFS install point. 
  I have C:\TFS\SRFN\MTU\PCLint vs. the default linsetup.bat:  C:\TFS\Firmware\SRFN\MTU\PCLint

Example run of lint from the command line:  
R:\>errors.bat Samwise_pjt ge_i210+
#########################################
Things you may need to do:  
-Slickedit was the default grep on my pc.  Modify LIN.BAT to use the GNU installed grep
   "C:\Program Files (x86)\GnuWin32\bin\grep" -v "cited in prior" R:\_LINT.TXT > R:\_LINT_GREP.TXT
-You might need to update your \SRFN\MTU\PCLint\Samwise.lnt if you don’t have IAR installed at the same place:  
 Update 7/13/2017: this is now handled in MyPath.lnt
   Example:  C:\Program Files (x86)\IAR Systems\Embedded Workbench 7.40.2\arm\inc\c
   vs:  C:\Program Files (x86)\IAR Systems\Embedded Workbench 7.2_2\arm\inc\c
   You’ll need to check-out and edit 4 lines to modify that file.  
-Had to also move a stdarg.h file from an old directory to the new directory, or ask for it from another dev 
   if you have a new PC install.  It goes in the C:\Program Files (x86)\IAR Systems\Embedded Workbench 7.2\arm\inc\c 
   directory.  
-Need to have LIN.bat and linsetup.bat in the environment variable path and NOT in current directory (R:))
LIN.bat is used, but does not appear to be in source control.  I have mine in:  C:\Program Files (x86)\GnuWin32\bin

it's contents are:  

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
