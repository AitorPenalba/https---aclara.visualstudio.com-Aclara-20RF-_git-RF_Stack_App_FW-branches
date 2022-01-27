echo off
rem To fill multiple ranges and produce a .hex file
rem General command format as used in IAR Project\Options...\Build actions\Post-build command line: $TOOLKIT_DIR$\bin\ielftool.exe "$TARGET_BPATH$.out" "$TARGET_BPATH$.hex" --fill 0xFF;0x4000-0x7DFFF;0x80000-0xFFFFF --ihex

rem Need to fill multiple ranges and produce both a .out and .hex files
rem General command format as used in IAR Project\Options...\Build actions\Post-build command line: $TOOLKIT_DIR$\bin\ielftool.exe "$TARGET_BPATH$.out" "$TARGET_BPATH$.out" --fill 0xFF;0x4000-0x7DFFF;0x80000-0xFFFFF
rem General command format as used in IAR Project\Options...\Build actions\Post-build command line: $TOOLKIT_DIR$\bin\ielftool.exe "$TARGET_BPATH$.out" "$TARGET_BPATH$.hex" --ihex

"%~1\bin\ielftool.exe" "%~2.out" "%~2.out" --fill 0xFF;0x4000-0x7DFFF;0x80000-0xFFFFF --silent
"%~1\bin\ielftool.exe" "%~2.out" "%~2.hex" --ihex --silent
