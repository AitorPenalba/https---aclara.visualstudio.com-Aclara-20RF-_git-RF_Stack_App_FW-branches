@echo off
rem Save current working directory in case the subst /d commands make it invalid, temporarily and switch to c:
pushd c:
subst i: /d
subst r: /d
subst L: /d
subst V: /d

rem Lint-nt.exe installation directory
subst L: C:\lint

rem Project lint directory
rem subst R: "C:\Clear Case\F1WVer.23_01-20-12\Lint"
rem subst R: "C:\Clear Case\F1 Water V17\Lint"
rem subst R: m:\mkv_Master_dyn\PRODUCT_DEVELOPMENT\RCE\UMT-R-P24EP\Lint
rem subst R: m:\mkv_Master_dyn\PRODUCT_DEVELOPMENT\RCE\DRU\Lint
rem subst r:  "V:\Users\DVelasco\Firmware image authentication\11-23-2011a baseline with HMACs - 15March2013 backup\Lint"
rem subst R: "C:\TFS Workspaces\MTU\2009-005_010\Lint"
rem subst R: "C:\TFS Workspaces\MTU\2013-005\Lint"
rem subst R: "C:\TFS Workspaces\MTU\Series4000Lite\Lint"
rem subst R: "C:\TFS Workspaces\9975-Main RC2e\Lint"
rem subst R: "C:\TFS Workspaces\9975J\Lint"
rem subst R: "C:\TFS Workspaces\RCE\UMT-R-F\lint"
rem subst R: "C:\TFS Workspaces\RCE\UMT-R-G2\lint"
rem subst R: "S:\Product Development Shared\RCE\msingla_folder\UMT-R-P24EP\Lint"
rem subst R: "C:\TFS Workspaces\RCE\UMT-R-P24EP\lint"
rem subst R: "C:\TFS Workspaces\TPLC\lint"
rem subst R: "C:\Users\mvirkus\TFS Workspaces\MTU\GE I210+\Aclara\VLF_Transponder\PcLint"
subst R: "C:\Projects\SRFN\Firmware\TFSSource\Tip\MTU\PCLint"
rem subst R: "C:\TFS\SRFN\DCU\Frodo\PCLint"
rem subst R: "C:\vm_shared\DCU3_MainBoard\PcLint"

rem project top level directory - include directives should be relative to this point
rem subst I: "C:\Clear Case\F1 Water V17"
rem subst I: "C:\Clear Case\F1WVer.23_01-20-12"
rem subst I: m:\mkv_Master_dyn\PRODUCT_DEVELOPMENT\RCE\UMT-R-P24EP
rem subst I: m:\mkv_Master_dyn\PRODUCT_DEVELOPMENT\RCE\DRU
rem subst I:  "V:\Users\DVelasco\Firmware image authentication\11-23-2011a baseline with HMACs - 15March2013 backup"
rem subst I: "C:\TFS Workspaces\MTU\2009-005_010"
rem subst I: "C:\TFS Workspaces\MTU\2013-005"
rem subst I: "C:\TFS Workspaces\MTU\Series4000Lite"
rem subst I: "C:\TFS Workspaces\9975-Main RC2e"
rem subst I: "C:\TFS Workspaces\9975J"
rem subst I: "C:\TFS Workspaces\RCE\UMT-R-F"
rem subst I: "C:\TFS Workspaces\RCE\UMT-R-G2"
rem subst I: "S:\Product Development Shared\RCE\msingla_folder\UMT-R-P24EP"
rem subst I: "C:\TFS Workspaces\RCE\UMT-R-P24EP"
rem subst I: "C:\TFS Workspaces\TPLC"
rem subst I: "C:\Users\mvirkus\TFS Workspaces\MTU\GE I210+\Aclara\VLF_Transponder"
rem subst V: "C:\Users\mvirkus\TFS Workspaces\MTU\GE I210+"
subst I: "C:\Projects\SRFN\Firmware\TFSSource\Tip\MTU"
rem subst I: "C:\TFS\SRFN\DCU\Frodo"
subst V: "C:\Projects\SRFN\Firmware\TFSSource\Tip"

rem return to original working directory
popd
