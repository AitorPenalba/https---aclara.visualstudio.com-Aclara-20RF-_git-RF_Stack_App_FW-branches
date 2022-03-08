"Program, Verify and run.  Select a .hex or .out file"

Add-Type -AssemblyName System.Windows.Forms
$refFile                  = new-object Windows.Forms.OpenFileDialog
$refFile.InitialDirectory = $PSScriptRoot
$refFile.Title            = "Select File"
$refFile.Filter           = "Intel hex Files (*.hex)|*.hex|Output Files (*.out)|*.out"
$refFile.ShowHelp         = $false
$refFile.Multiselect      = $false
[void]$refFile.ShowDialog()

$GeneralXcl   = "$PSScriptRoot\ProgVerifyRun.general.xcl"
$DebugFileXcl = "$PSScriptRoot\ProgVerifyRun.File.xcl"
$DriverXcl    = "$PSScriptRoot\ProgVerifyRun.driver.xcl"
$FlashLoader  = "$PSScriptRoot\Flash_EP.board"

$name=$refFile.FileName
$name="`"$name`""

if ($name -eq "`"`"")
{
   "File not selected, aborting"
   return
}

# Write filename to the xcl file
#  This was done since PowerShell REALLY does not work well when the path\filename has a space - PS will either put quotes around all of the parameter or split into two parameters
#  i.e. "--debug_file="C:\Projects\SRFN\Firmware\TFSSource\Latest\MTU\IAR\GE_I210+ Debug\Exe\Y84020-1-V1.40.xx.hex""
#    or  --debug_file= "C:\Projects\SRFN\Firmware\TFSSource\Latest\MTU\IAR\GE_I210+ Debug\Exe\Y84020-1-V1.40.xx.hex"
#  obviously neither works with cspybat
Set-Content $DebugFileXcl $name

"Using I-Jet Probe SN: 81838."
$Drv_Comm = "`"--drv_communication=USB:#81838`""

"Program and Verify device."
#Use this to check param: echoargs -f $Generalxcl -f $DebugFileXcl --flash_loader $FlashLoader --leave_running --backend -f $DriverXcl

& "C:\Program Files (x86)\IAR Systems\Embedded Workbench 7.2\common\bin\cspybat" -f $Generalxcl -f $DebugFileXcl --flash_loader $FlashLoader --leave_running --backend -f $DriverXcl $Drv_Comm

"Program\Verify\Run Complete!"
