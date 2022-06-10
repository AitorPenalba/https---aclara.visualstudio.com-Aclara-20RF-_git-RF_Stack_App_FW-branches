import os
# import sys
# import shutil
# from FirmwareBuilder import FirmwareBuilder
# from fileGlobals import init_file_globals, set_file_global, get_file_global
# from xmlBuilder.xmlBuilder import build_xml_files
# from utility import ArtifactFolders, get_fw_version
# from doxygenBuilder.doxygenBuilder import build_doxygen_files
# from LintChecker.LintChecker import run_pc_lint
import json

########################################################################################################
#
#
#  The following functions are from fileGlobals.py
#  On all functions I've doubled the last character to separated  from the real build server
#
########################################################################################################


def init_file_globals():
    # this gets environ path and sees if it is an absolute path and,
    # set it to an absolut if its not, then prints 'Global'path
    path = os.environ["environ_path"]
    if not os.path.isabs(path):
        path = os.path.abspath(path)
        os.environ["environ_path"] = path

    print(f"File Globals Path = {path}")

    # checks for a valid file path, or file ??  and writes to 'file_globals'
    if not os.path.exists(path):
        with open(path, 'w') as outfile:
            file_globals = {}
            json.dump(file_globals, outfile)


def get_file_global(name):
    # opens 'file_globals' file and reads depending on the name passed in
    with open(os.environ["environ_path"], 'r') as f:
        file_globals = json.loads(f.read())

    return file_globals[name]


def set_file_global(name, value):
    # name and value are passed in and the are written to the JSon file
    # also prints that is has set the file global.
    with open(os.environ["environ_path"], 'r') as f:
        file_globals = json.loads(f.read())

    file_globals[name] = value

    print(f"Setting file global {name}={value}")

    with open(os.environ["environ_path"], 'w') as f:
        f.write(json.dumps(file_globals))




def main():

    enviro_file_path = "C:\\temp\\TRY_build_server_environment_variables.txt"
    if os.path.exists(enviro_file_path):
        # checks to see if we have a environ path leftover from last run, if so it deletes it with the next line.
        os.remove(enviro_file_path)

    os.environ["environ_path"] = "C:\\temp\\TRY_build_server_environment_variables.txt"
    init_file_globals()

    set_file_global(name='root_dir', value=os.getcwd())
    set_file_global(name='project_path', value="C:\\Projects\\Firmware\\SRFN_TIP")
    #set_file_global(name='mtu_com_port', value="COM4")
    set_file_global(name='mfg_com_port', value="COM4")
    set_file_global(name='dbg_com_port', value="COM8")
    set_file_global(name='path_to_iar_build_exe', value="C:\\Program Files (x86)\\IAR Systems\\Embedded Workbench 7.2\\common\\bin\\IarBuild.exe")
    set_file_global(name='path_to_version_dot_h', value="C:\\Projects\\Firmware\\SRFN_TIP\\MTU\\Common")
    set_file_global(name='firmware_family', value="99")
    # TODO  mrh firmware family is hardcoded to 99 to indicate SRFN, is this the final fix???

    # set_file_global(name='firmware_family', value="2")
    # set_file_global(name='path_to_srec_cat_exe', value="C:\\Development\\Tools\\srecord\\srec_cat.exe")
    # set_file_global(name='path_to_doxygen_exe', value="C:\\Program Files\\doxygen\\bin\\doxygen.exe")
    # set_file_global(name='path_to_pclint', value="C:\\PCLint\\pclp64.exe")
    # set_file_global(name='path_to_uniflash_batch_file', value="C:\\ti\\uniflash_windows_CLI\\dslite.bat")
    # set_file_global(name='path_to_uniflash_ccxml', value="C:\\ti\\uniflash_windows_CLI\\MSP430FR5964.ccxml")

if __name__ == "__main__":
        main()