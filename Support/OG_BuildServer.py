import os
import sys
import shutil
from FirmwareBuilder import FirmwareBuilder
from fileGlobals import init_file_globals, set_file_global, get_file_global
from xmlBuilder.xmlBuilder import build_xml_files
from utility import ArtifactFolders, get_fw_version
from doxygenBuilder.doxygenBuilder import build_doxygen_files
from LintChecker.LintChecker import run_pc_lint


if __name__ == "__main__":
    if len(sys.argv) == 1:
        # no arguments indicates we should run through the entire process, on a developer's machine
        phase = None

        # set these correctly based on your local machine's settings

        enviro_file_path = "C:\\temp\\build_server_environment_variables.txt"
        if os.path.exists(enviro_file_path):
            # checks to see if we have a environ path leftover from last run, if so it deletes it with the next line.
            os.remove(enviro_file_path)

        os.environ["environ_path"] = "C:\\temp\\build_server_environment_variables.txt"
        init_file_globals()

        set_file_global(name='root_dir', value=os.getcwd())
        set_file_global(name='project_path', value="C:\\Development\\STAR\\MTU\\Gen3_Products\\GasAndWaterMtus2017\\Main")
        set_file_global(name='firmware_family', value="2")
        set_file_global(name='mtu_com_port', value="COM5")
        set_file_global(name='path_to_uniflash_batch_file', value="C:\\ti\\uniflash_windows_CLI\\dslite.bat")
        set_file_global(name='path_to_uniflash_ccxml', value="C:\\ti\\uniflash_windows_CLI\\MSP430FR5964.ccxml")
        set_file_global(name='path_to_iar_build_exe', value="C:\\Program Files (x86)\\IAR Systems\\Embedded Workbench 8.4\\common\\bin\\IarBuild.exe")
        set_file_global(name='path_to_srec_cat_exe', value="C:\\Development\\Tools\\srecord\\srec_cat.exe")
        set_file_global(name='path_to_doxygen_exe', value="C:\\Program Files\\doxygen\\bin\\doxygen.exe")
        set_file_global(name='path_to_pclint', value="C:\\PCLint\\pclp64.exe")

    elif len(sys.argv) == 2:
        # if first argument is present, it indicates which phase to run on the build server
        phase = int(sys.argv[1])

        init_file_globals()

        # get pipeline variables (set in the "Variables" tab)
        if phase == 6:
            set_file_global(name='project_path', value=os.environ['project_path'])
            set_file_global(name='firmware_family', value=os.environ['firmware_family'])
            set_file_global(name='path_to_signatures', value=os.environ['path_to_signatures'])
            set_file_global(name='path_to_srec_cat_exe', value=os.environ['path_to_srec_cat_exe'])
            set_file_global(name='dfw_target_mtu_type', value=os.environ['dfw_target_mtu_type'])
            set_file_global(name='dfw_target_fw_major', value=os.environ['dfw_target_fw_major'])
            set_file_global(name='dfw_target_fw_minor', value=os.environ['dfw_target_fw_minor'])
            set_file_global(name='dfw_target_fw_build', value=os.environ['dfw_target_fw_build'])

        else:
            set_file_global(name='root_dir', value=os.getcwd())
            set_file_global(name='project_path', value=os.environ['project_path'])
            set_file_global(name='firmware_family', value=os.environ['firmware_family'])
            set_file_global(name='mtu_com_port', value=os.environ['mtu_com_port'])
            set_file_global(name='path_to_uniflash_batch_file', value=os.environ['path_to_uniflash_batch_file'])
            set_file_global(name='path_to_uniflash_ccxml', value=os.environ['path_to_uniflash_ccxml'])
            set_file_global(name='path_to_iar_build_exe', value=os.environ['path_to_iar_build_exe'])
            set_file_global(name='path_to_srec_cat_exe', value=os.environ['path_to_srec_cat_exe'])
            set_file_global(name='path_to_doxygen_exe', value=os.environ['path_to_doxygen_exe'])
            set_file_global(name='project_path', value=os.environ['project_path'])
            set_file_global(name='path_to_pclint', value=os.environ['path_to_pclint'])

    else:
        raise RuntimeError()

    # ------------------------------------------------------------------------------------------------------------------
    project_path = os.path.abspath(get_file_global(name='project_path'))

    if int(get_file_global(name='firmware_family')) == 0:
        set_file_global(name='release_name', value="GasGeneral")
        set_file_global(name='path_to_version_dot_h', value=os.path.join(project_path, "Gas_Version.h"))
        set_file_global(name='path_to_doxygen_conf',
                        value=os.path.join(project_path, "support", "GasGeneralDoxyfile"))
        set_file_global(name='iar_app_target_name', value="Gas_Release")
    elif int(get_file_global(name='firmware_family')) == 1:
        set_file_global(name='release_name', value="GasPulseEvc")
        set_file_global(name='path_to_version_dot_h', value=os.path.join(project_path, "GasPulseEvc_Version.h"))
        set_file_global(name='path_to_doxygen_conf',
                        value=os.path.join(project_path, "support", "GasPulseEvcDoxyfile"))
        set_file_global(name='iar_app_target_name', value="GasPulseEvc_Release")
    elif int(get_file_global(name='firmware_family')) == 2:
        set_file_global(name='release_name', value="WaterGeneral")
        set_file_global(name='path_to_version_dot_h', value=os.path.join(project_path, "Encoder_Version.h"))
        set_file_global(name='path_to_doxygen_conf',
                        value=os.path.join(project_path, "support", "EncoderDoxyfile"))
        set_file_global(name='iar_app_target_name', value="Encoder_Release")
    elif int(get_file_global(name='firmware_family')) == 3:
        set_file_global(name='release_name', value="WaterAdvAlarms")
        set_file_global(name='path_to_version_dot_h', value=os.path.join(project_path, "AdvAlarms_Version.h"))
        set_file_global(name='path_to_doxygen_conf',
                        value=os.path.join(project_path, "support", "AdvAlarmsDoxyfile"))
        set_file_global(name='iar_app_target_name', value="AdvAlarms_Release")
    else:
        raise RuntimeError("Unsupported f/w family")

    # ------------------------------------------------------------------------------------------------------------------
    if (phase is None) or (phase == 0):
        get_fw_version(os.path.abspath(get_file_global('path_to_version_dot_h')))

        artifact_folders = ArtifactFolders(project_path=os.path.abspath(get_file_global('project_path')),
                                           release_name=get_file_global('release_name'),
                                           firmware_family=get_file_global('firmware_family'),
                                           version_string=get_file_global('version_string'))

    if (phase is None) or (phase == 1):
        builder = FirmwareBuilder()
        builder.build_firmware()

    if (phase is None) or (phase == 2):
        build_xml_files(fw_family=int(get_file_global('firmware_family')),
                        version_major=int(get_file_global("fw_major")),
                        version_minor=int(get_file_global("fw_minor")),
                        version_build=int(get_file_global("fw_build")),
                        appCrc=int(get_file_global("app_crc")),
                        xml_gen_dir=os.path.abspath(get_file_global(name='xml_dir')))

    if (phase is None) or (phase == 3):
        run_pc_lint(path_to_output_dir=os.path.abspath(get_file_global(name='pc_lint_dir')))

    if (phase is None) or (phase == 4):
        build_doxygen_files(path_to_doxygen_run_from_dir=os.path.abspath(os.path.join(project_path, "support")),
                            path_to_doxygen_output_dir=os.path.abspath(get_file_global(name='doxygen_dir')))

    if (phase is None) or (phase == 5):
        # make a zip file of just the doxygen files
        doxygen_dir = os.path.abspath(get_file_global('doxygen_dir'))
        os.chdir(os.path.join(doxygen_dir, ".."))
        zip_name = "doxygen"
        shutil.make_archive(zip_name, 'zip', doxygen_dir)
        doxygen_zip_path = os.path.join(os.path.dirname(doxygen_dir), zip_name + ".zip")
        new_doxygen_zip_path = os.path.join(os.environ["BUILD_ARTIFACTSTAGINGDIRECTORY"], zip_name + ".zip")
        shutil.move(doxygen_zip_path, new_doxygen_zip_path)

        # now delete the doxygen directory (it is too big to include in the "everything" zip file)
        shutil.rmtree(path=doxygen_dir)

        # make a zip file of everything
        build_dir = os.path.abspath(get_file_global('build_dir'))
        zip_name = os.path.basename(build_dir)
        os.chdir(os.path.join(build_dir, ".."))
        shutil.make_archive(zip_name, 'zip', build_dir)
        everything_zip_path = os.path.join(os.path.dirname(build_dir), zip_name + ".zip")

        # get the paths of other locations we will publish as independent artifacts
        os.chdir(get_file_global(name='root_dir'))
        xml_path = os.path.abspath(get_file_global(name='xml_dir'))
        mfg_image_path = os.path.abspath(get_file_global(name='mfg_image_path'))
        unit_test_path = os.path.abspath(get_file_global(name='unit_test_dir'))

        if (phase is not None):
            # this line updates the build server variable...
            print(f'##vso[task.setvariable variable=everything_zip_path]{everything_zip_path}')
            print(f'##vso[task.setvariable variable=build_dir]{build_dir}')
            print(f'##vso[task.setvariable variable=doxygen_zip_path]{new_doxygen_zip_path}')
            print(f'##vso[task.setvariable variable=xml_path]{xml_path}')
            print(f'##vso[task.setvariable variable=mfg_image_path]{mfg_image_path}')
            print(f'##vso[task.setvariable variable=unit_test_path]{unit_test_path}')

    if (phase is None) or (phase == 6):
        builder = FirmwareBuilder(signing_only=True)
        builder.sign_firmware()

        shutil.rmtree(path=os.path.join(os.environ["AGENT_BUILDDIRECTORY"], "stage_1_artifacts", "temp"))
        shutil.rmtree(path=os.path.join(os.environ["AGENT_BUILDDIRECTORY"], "stage_1_artifacts", "unit_test_images"))
        shutil.rmtree(path=os.path.join(os.environ["AGENT_BUILDDIRECTORY"], "stage_1_artifacts", "PC_Lint_Output"))
        shutil.rmtree(path=os.path.join(os.environ["AGENT_BUILDDIRECTORY"], "stage_1_artifacts", "build_artifacts"))
        shutil.rmtree(path=os.path.join(os.environ["AGENT_BUILDDIRECTORY"], "stage_1_artifacts", "files_to_be_signed"))
