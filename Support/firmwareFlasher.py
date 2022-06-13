import os
import subprocess
import parse
from fileGlobals import init_file_globals, set_file_global, get_file_global
import sys

def flash_image(path_to_image):
    """
    This function can be used to program an MTU with a manufacturing image.

    It is just a wrapper for the TI Uniflash 4.4.0 command line interface.

    To use this class some setup is required:
    1) Connect the MSP-FET to the MTU.
    2) Start UniFlash 4.4.0, ensure it autodetects the MTU, click Start
    3) In the left hand pane, click "Standalone Command Line"
        a. Deselect the Images checkbox
        b. Make sure other settings are reasonable.
        c. Click "Generate Package"
        d. This will generate a .zip file, save it to a convenient location
    4) Click the "download ccxml" button in UniFlash (this is at the top of the window, toward the center)
        a. Save the .ccxml file to a convenient location
    5) Close UniFlash
    6) Extract the zip file created in step 3 to a convenient location.
    7) You may need to run "one_time_setup.bat"

    """

    response = subprocess.call([os.path.abspath(get_file_global('path_to_uniflash_batch_file')),
                                "-c",
                                os.path.abspath(get_file_global('path_to_uniflash_ccxml')),
                                "-f",
                                path_to_image])

    return response  # 0 means success

def flash_SRFN_image():

    # TODO: I added the below line to test flash functions without building each time,
    #  route to a differant build time N date
    #  build_artifacts_dir = "C:\\build\\VSTS-Agent1\\44\\s\\ReleaseFiles\\SFRN_FW_Release_fwFamily99_02.00.0081_2022-05-02-12-02-20\\build_artifacts"

    project_path= get_file_global(name="project_path")
    path_to_flasher = "C:\\Program Files (x86)\\IAR Systems\\Embedded Workbench 7.2\\common\\bin\\cspybat"
    log_file_name = "I-Jet flash log.txt"
    # log_file_path = "C:\\RegressionTestTool\\"
    # TODO complete path for I-jet test log
    log_file_path = os.path.join(os.path.abspath(get_file_global(name='build_artifacts_dir')), log_file_name)
    # log_file_path = os.path.join(os.path.abspath(get_file_global(name='build_artifacts_dir')), log_file_name)
    # log_file_path = os.path.join(project_path, ,log_file_name)
    pc_name = str(get_file_global(name="computer_name"))

    # TODO MRH: later this if could be used for the rack and checking for firmware type

    # TODO EE: add paths for the T Board on both the Test bench and rack
    # right now the file paths listed are passed on I210+c/Kv2c and following that structure in the solution explorer
    if pc_name == 'ACL10350D':  # Test bench
        if str(get_file_global(name="firmware_type_ep")) == "I210+c":
            path_to_flash_general = os.path.join(project_path, "MTU", "IAR", "AutoProgram",
                                                 "GE_I210+c.Debug.general.xcl")
            path_to_flash_driver = os.path.join(project_path, "MTU", "IAR", "AutoProgram", "GE_I210+c.Debug.driver.xcl")
            ijet_sn = "--drv_communication=USB:#83681"
        elif str(get_file_global(name="firmware_type_ep")) == "KV2c":
            path_to_flash_general = os.path.join(project_path, "MTU", "IAR", "AutoProgram", "GE_KV2c.Debug.general.xcl")
            path_to_flash_driver = os.path.join(project_path, "MTU", "IAR", "AutoProgram", "GE_KV2c.Debug.driver.xcl")
            ijet_sn = "--drv_communication=USB:#94076"
        elif str(get_file_global(name = "firmware_type_ep")) == "T-Board":
            if str(get_file_global(name = "T-Board type")) == "9975T":
                path_to_flash_general = os.path.join(project_path, "DCU", "IAR", "AutoProgram", "Frodo.Debug.general.xcl") # check this
                path_to_flash_driver = os.path.join(project_path, "DCU", "IAR", "AutoProgram", "Frodo.Debug.driver.xcl") # check this
                ijet_sn = "--drv_communication=USB:#91843" # Test bench's Ijet
            else: ## 9985T
                path_to_flash_general = os.path.join(project_path, "DCU", "IAR", "AutoProgram", "Frodo_9985T.Debug.general.xcl") # check this here
                path_to_flash_driver = os.path.join(project_path, "DCU", "IAR", "AutoProgram", "Frodo_9985T.Debug.driver.xcl") # check this
                ijet_sn = "" # not yet implemented
        else:
            pass
    elif pc_name == 'STL10052D':  # The rack
        if str(get_file_global(name="firmware_type_ep")) == "I210+c":
            path_to_flash_general = os.path.join(project_path, "MTU", "IAR", "AutoProgram",
                                                 "GE_I210+c.Debug.general.xcl")
            path_to_flash_driver = os.path.join(project_path, "MTU", "IAR", "AutoProgram", "GE_I210+c.Debug.driver.xcl")
            ijet_sn = "--drv_communication=USB:#84372" # Racks's I210+c Ijet
        elif str(get_file_global(name="firmware_type_ep")) == "KV2c":
            path_to_flash_general = os.path.join(project_path, "MTU", "IAR", "AutoProgram", "GE_KV2c.Debug.general.xcl")
            path_to_flash_driver = os.path.join(project_path, "MTU", "IAR", "AutoProgram", "GE_KV2c.Debug.driver.xcl")
            ijet_sn = "--drv_communication=USB:#84284" # Racks's KV2c Ijet
        elif str(get_file_global(name="firmware_type_ep")) == "KV2c":
            path_to_flash_general = os.path.join(project_path, "MTU", "IAR", "AutoProgram", "GE_KV2c.Debug.general.xcl")
            path_to_flash_driver = os.path.join(project_path, "MTU", "IAR", "AutoProgram", "GE_KV2c.Debug.driver.xcl")
            ijet_sn = "--drv_communication=USB:#94076"
        elif str(get_file_global(name = "firmware_type_ep")) == "T-Board":
            if str(get_file_global(name = "T-Board type")) == "9975T":
                path_to_flash_general = os.path.join(project_path, "DCU", "IAR", "AutoProgram", "Frodo.Debug.general.xcl") # check this
                path_to_flash_driver = os.path.join(project_path, "DCU", "IAR", "AutoProgram", "Frodo.Debug.driver.xcl") # check this
                ijet_sn = "" # not implemented yet
            else: ## 9985T
                path_to_flash_general = os.path.join(project_path, "DCU", "IAR", "AutoProgram", "Frodo_9985T.Debug.general.xcl") # check this here
                path_to_flash_driver = os.path.join(project_path, "DCU", "IAR", "AutoProgram", "Frodo_9985T.Debug.driver.xcl") # check this
                ijet_sn = "" # not implemented yet
        else:
            # TODO MRH: We dont have code yet for the Tbrd build, and flash, but here is the Ijet
            # ijet_sn = "--drv_communication=USB:#80285"  # Racks's T-board Ijet
            pass


    else:
        pass

    path_to_macros = os.path.join(project_path, "MTU", "IAR", "SRFN_DebugMacros.mac")
    path_to_flashepboard = os.path.join(project_path, "MTU", "IAR", "Flash_EP.board")



    with open(log_file_path, 'w') as f:
        functionParams = [path_to_flasher, "-f",
                          path_to_flash_general, "--macro",
                          path_to_macros, "--flash_loader",
                          path_to_flashepboard, "--leave_running", "--backend", "-f",
                          path_to_flash_driver, ijet_sn]

                          # "C:\\agents\\Firmware_3\\_work\\6\\s\\MTU\\IAR\\Flash_EP.board", "--leave_running", "--backend", "-f",
                          # path_to_flash_driver, ijet_sn]
                          # "C:\Projects\Firmware\SRFN_Tip2\MTU\IAR\AutoProgram\GE_I210+c.Debug.driver.xcl",
                          # "--drv_communication = USB :#92149", "log"]

        # TODO: This file should be changed to re-direct to the out file to program # "C:\Projects\Firmware\SRFN_Tip2\MTU\IAR\AutoProgram\GE_I210+c.Debug.general.xcl"
        # TODO:  I need to find out why I'm not getting my whole log

        # we use subprocess to run the iar compiler, paramters are passed in with functionParams
        process = subprocess.Popen(functionParams, stdout=subprocess.PIPE, encoding='utf_8')
        # TODO:  I need to wrap this in a try statment

        for line in iter(process.stdout.readline, ''):
            f.write(line)
            print(f"{log_file_name}_output log >>>>> {line.strip()}")
            # print("We Just flashed FW")

            # TODO:  I dont believe the the below logic works, I was wrapped up in getting the flash working
            #   address the log issue also
            if "failed" in line:
                print("Failed")

            if "Kinetis" in line:
                print("Passed")


    return # response  # 0 means success
