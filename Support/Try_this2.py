import os
import subprocess
import parse
from fileGlobals import init_file_globals, set_file_global, get_file_global
import sys


def main():

    path_to_flasher = "C:\\Program Files (x86)\\IAR Systems\\Embedded Workbench 7.2\\common\\bin\\cspybat"
    log_file_name = "I-Jet flash log.txt"
    # log_file_path = "C:\\RegressionTestTool\\"
    log_file_path = os.path.join("C:\RegressionTestTool", log_file_name)

    # path_to_flasher = get_file_global('path_to_I-jet_flash')
    pc_name = "ACL11029L"

    if (pc_name) == 'ACL11029L':# mylaptop
        ijet_sn = "--drv_communication = USB :#83681"
    elif pc_name =='ACL10350D': # my desktop
        ijet_sn = "--drv_communication = USB :#83681"
    else:
        pass




    with open(log_file_path, 'w') as f:
        functionParams = [path_to_flasher, "-f", "C:\Projects\Firmware\SRFN_Tip2\MTU\IAR\AutoProgram\GE_I210+c.Debug.general.xcl", "--macro",
                      "C:\Projects\Firmware\SRFN_Tip2\MTU\IAR\SRFN_DebugMacros.mac", "--flash_loader",
                      "C:\Projects\Firmware\SRFN_Tip2\MTU\IAR\Flash_EP.board", "--leave_running", "--backend", "-f",
                      # "C:\Projects\Firmware\SRFN_Tip2\MTU\IAR\AutoProgram\GE_I210+c.Debug.driver.xcl", "--drv_communication = USB :#83681"]
                      "C:\Projects\Firmware\SRFN_Tip2\MTU\IAR\AutoProgram\GE_I210+c.Debug.driver.xcl", ijet_sn,"log"]
        # TODO: This file should be changed to re-direct to the out file to program # "C:\Projects\Firmware\SRFN_Tip2\MTU\IAR\AutoProgram\GE_I210+c.Debug.general.xcl"
        # TODO:  I need to find out why I'm not getting my whole log


        # we use subprocess to run the iar compiler, paramters are passed in with functionParams
        process = subprocess.Popen(functionParams, stdout=subprocess.PIPE, encoding='utf_8')
        # TODO:  I need to wrap this in a try statment

        for line in iter(process.stdout.readline, ''):
            f.write(line)
            print(f"{log_file_name}_output log >>>>> {line.strip()}")
           # print("We Just flashed FW")



            if "failed" in line:
                print("Failed")

            if "Kinetis" in line:
                print("Passed")








if __name__ == "__main__":
        main()