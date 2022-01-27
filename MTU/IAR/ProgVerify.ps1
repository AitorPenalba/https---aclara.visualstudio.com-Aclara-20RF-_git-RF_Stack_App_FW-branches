"Program Verify, select a .hex or .out file"

Add-Type -AssemblyName System.Windows.Forms
$refFile                  = new-object Windows.Forms.OpenFileDialog
$refFile.InitialDirectory = $PSScriptRoot
$refFile.Title            = "Select File"
$refFile.Filter           = "Intel hex Files (*.hex)|*.hex|Output Files (*.out)|*.out"
$refFile.ShowHelp         = $false
$refFile.Multiselect      = $false
[void]$refFile.ShowDialog()

$GeneralXcl   = "$PSScriptRoot\ProgVerify.general.xcl"
$DebugFileXcl = "$PSScriptRoot\ProgVerify.File.xcl"
$DriverXcl    = "$PSScriptRoot\ProgVerify.driver.xcl"

$name=$refFile.FileName
$name="`"$name`""

if ($name -eq "`"`"")
{
   "File not selected, aborting"
   return
}
# Write filename to the xcl file
#  This was done since PowerShell REALLY does not work well when the path/filename has a space - PS will either put qoutes around all of the parameter or split into two parameters
#  i.e. "--debug_file="C:\Projects\SRFN\Firmware\TFSSource\Latest\MTU\IAR\GE_I210+ Debug\Exe\Y84020-1-V1.40.xx.hex""
#    or  --debug_file= "C:\Projects\SRFN\Firmware\TFSSource\Latest\MTU\IAR\GE_I210+ Debug\Exe\Y84020-1-V1.40.xx.hex"
#  obviously neither works with cspybat
Set-Content $DebugFileXcl $name

"Executing the Program Verify.  This may take up to 30 seconds..."
#Use this to check param: echoargs -f $Generalxcl -f $DebugFileXcl --backend -f $DriverXcl 

& "C:\Program Files (x86)\IAR Systems\Embedded Workbench 7.2\common\bin\cspybat" -f $Generalxcl -f $DebugFileXcl --backend -f $DriverXcl 

"Verify Complete!"
"If `"WARNING: Download completed but verification failed.`" is displayed, the Download completed part can be ignored."
"  Code was not actually downloaded, existing code was only verified to the selected file."
