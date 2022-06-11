##############################################################################
#  *                                  SRFN BUILD SERVER
#  *
#  * A product of
#  * Aclara Technologies LLC
#  * Confidential and Proprietary
#  * Copyright 2013-2022 Aclara.  All Rights Reserved.
#  *
#  * PROPRIETARY NOTICE
#  * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
#  * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
#  * authorization of Aclara.  Any software or firmware described in this document is furnished under a license
#  * and may be used or copied only in accordance with the terms of such license.
#
##############################################################################
#
# Entry point for the SRFN_Build_Server module (start of code execution)
#
# The "SRFN Build server" is a child of "Build Server" which was written by (Doug George)for the MTU product line.
# The "SRFN Build Server" was implemented by (Maurice Harris) to be used on the RF_Stack_App collection of branches.  It
# has been implemented in a way to still serve as "THE" build server.  So MTU products should still work using
# this version.  Any improvements should be mindful of that fact.
#
##############################################################################


import os
import sys
import shutil
from FirmwareBuilder import FirmwareBuilder
from fileGlobals import init_file_globals, set_file_global, get_file_global
from xmlBuilder.xmlBuilder import build_xml_files
from utility import ArtifactFolders, get_fw_version, get_SRFN_fw_version
from doxygenBuilder.doxygenBuilder import build_doxygen_files
from LintChecker.LintChecker import run_pc_lint
# TODO : take out the below firmwareflasher inint
from firmwareFlasher import flash_image


SRFN_Build_Server_ver = '1.2.13'


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

        set_file_global(name='SRFN_Build_Server_ver', value=SRFN_Build_Server_ver)
        set_file_global(name='root_dir', value=os.getcwd())
        agent_root_dir=str(os.getcwd())
        # TODO mrh I need to understand the argv/phase stuff so I can make the program more portable to run on PC only.
        # TODO mrh also pipeline variables
        # agent_real_root= agent_root_dir[:-7]  # remove this comment to run on the agent machine
        # TODO MRH add to main code to run without an agent
      #  if os.environ[('COMPUTERNAME')] == 'ACL11029L' or 'HUB3LC3533':
            # TODO firmware_family = 'SRFN'
       #     os.environ['firmware_family'] = '99'  # SRFN
            # TODO firmware_type_ep 1-'I210+c',2-'KV2c',3-'DA',4-'LC',5-'RA6',6-'T-Brd'
        #    os.environ['firmware_type_ep'] = '1'
            # TODO operation 1-'build', only,2-'build_flash', 3-'build_flash_test'
         #   os.environ['operation'] = '1'
          #  agent_real_root = agent_root_dir[:-7]  # remove this comment to run on the agent machine
           # agent_root_dir=agent_real_root


        # if the computernames are agents in the Firmware group we setup for a SRFN run
        # TODO MRH:  Why is the rack not failing? how is it getting its vars?
        if os.environ[('COMPUTERNAME')] == 'ACL10350D' or 'ACL11029L':
            # set_file_global(name ='project_path', value= os.path.join(agent_root_dir,"./"))
            set_file_global(name='project_path', value=agent_root_dir) # changes value to agent_real_root to run on the agent machine
            # TODO  mrh firmware family is hardcoded to 99 to indicate SRFN, is this the final fix???  Later I found how to pass in variables from the pipeline
            # this bring in variables set in the pipeline
            set_file_global(name='firmware_family', value=os.environ['firmware_family'])
            set_file_global(name='firmware_type_ep', value=os.environ['firmware_type_ep'])
            set_file_global(name="operation", value=os.environ['operation'])
            set_file_global(name='path_to_uniflash_batch_file', value="C:\\ti\\uniflash_windows_CLI\\dslite.bat")
            set_file_global(name='path_to_uniflash_ccxml', value="C:\\ti\\uniflash_windows_CLI\\MSP430FR5964.ccxml")
            if get_file_global(name='firmware_type_ep')== "RA6":
                set_file_global(name='path_to_iar_build_exe',
                                value="C:\\Program Files (x86)\\IAR Systems\\Embedded Workbench 9.0\\common\\bin\\IarBuild.exe")
                set_file_global(name='path_to_I-jet_flash',
                                value="C:\\Program Files (x86)\\IAR Systems\\Embedded Workbench 9.0\\common\\bin\\cspybat")
            else:
                set_file_global(name='path_to_iar_build_exe',
                                value="C:\\Program Files (x86)\\IAR Systems\\Embedded Workbench 7.2\\common\\bin\\IarBuild.exe")
                set_file_global(name='path_to_I-jet_flash',
                                value="C:\\Program Files (x86)\\IAR Systems\\Embedded Workbench 7.2\\common\\bin\\cspybat")
            set_file_global(name='path_to_srec_cat_exe', value="C:\\Development\\Tools\\srecord\\srec_cat.exe")
            set_file_global(name='path_to_doxygen_exe', value="C:\\Program Files\\doxygen\\bin\\doxygen.exe")
            set_file_global(name='path_to_pclint', value="C:\\PCLint\\pclp64.exe")
            set_file_global(name='mtu_com_port', value="COM5")

        else:
            set_file_global(name='project_path', value="C:\\Development\\STAR\\MTU\\Gen3_Products\\GasAndWaterMtus2017\\Main")
            set_file_global(name='firmware_family', value="2")
            set_file_global(name='mtu_com_port', value="COM5")
            set_file_global(name='path_to_uniflash_batch_file', value="C:\\ti\\uniflash_windows_CLI\\dslite.bat")
            set_file_global(name='path_to_uniflash_ccxml', value="C:\\ti\\uniflash_windows_CLI\\MSP430FR5964.ccxml")
            set_file_global(name='path_to_iar_build_exe',
                            value="C:\\Program Files (x86)\\IAR Systems\\Embedded Workbench 8.4\\common\\bin\\IarBuild.exe")
            set_file_global(name='path_to_srec_cat_exe', value="C:\\Development\\Tools\\srecord\\srec_cat.exe")
            set_file_global(name='path_to_doxygen_exe', value="C:\\Program Files\\doxygen\\bin\\doxygen.exe")
            set_file_global(name='path_to_pclint', value="C:\\PCLint\\pclp64.exe")

        # path_to_flasher = get_file_global('path_to_I-jet_flash')

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
    elif int(get_file_global(name='firmware_family')) == 99:
        # TODO these values are just placeholders until I decide how to intergrate SRFN in the server, and if
        #  these fields are needed
        set_file_global(name='release_name', value="SFRN_FW_Release")
        # set_file_global(name='path_to_iar_build_exe',value="C:\\Program Files (x86)\\IAR Systems\\Embedded Workbench 7.2\\common\\bin\\IarBuild.exe")
        # set_file_global(name='path_to_I-jet_flash', value="C:\\Program Files (x86)\\IAR Systems\\Embedded Workbench 7.2\\common\\bin\\cspybat")
        # set_file_global(name='path_to_version_dot_c', value=os.path.join(project_path, "version.c"))
        # TODO: MRH  I'm not sure if I needed to set project path twice, I set project path if computer name is l..350
        # set_file_global(name ='project_path', value= os.path.join(agent_root_dir,"./"))
        # TODO firmware_type 1-I210+c,2-KV2c,3-DA,4-LC,5-RA6,6T-Brd
        # set_file_global(name='firmware_type', value=os.environ['firmware_type'])
        # TODO operation 1-build only,2-build and flash, 3-build, flash, and test
        # set_file_global(name="operation",value="2") this is read in as a ADO var, instead of this hard code
        set_file_global(name='mfg_com_port', value="COM4")
        set_file_global(name='dbg_com_port', value="COM8")
        set_file_global(name='path_to_doxygen_conf',
                        value=os.path.join(project_path, "Support", "SRFN_doxygen"))
        set_file_global(name='iar_app_target_name', value="Debug")
        # TODO MRH: fix this file path to the, it needs the project path
        # set_file_global(name='path_to_version_dot_h', value="C:\\Projects\\Firmware\\SRFN_TIP2\\MTU\\Common\\version.h")
        # set_file_global(name='path_to_version_dot_c', value="C:\\Projects\\Firmware\\SRFN_TIP2\\MTU\\Common\\version.c")
        set_file_global(name='path_to_version_dot_h', value=os.path.join( project_path,"MTU","Common", "version.h"))
        set_file_global(name='path_to_version_dot_c', value=os.path.join( project_path,"MTU","Common", "version.c"))
        set_file_global(name='computer_name', value=os.environ['COMPUTERNAME'])

        # the below line is for xml fake out becasue we dont have a CRC
        # set_file_global(name='app_crc', value="4856")


    else:
        raise RuntimeError("Unsupported f/w family")

    # ------------------------------------------------------------------------------------------------------------------
    if (phase is None) or (phase == 0):
        if int(get_file_global(name='firmware_family')) == 99:
            get_SRFN_fw_version(os.path.abspath(get_file_global('path_to_version_dot_c')))

            artifact_folders = ArtifactFolders(project_path=os.path.abspath(get_file_global('project_path')),
                                           release_name=get_file_global('release_name'),
                                           firmware_family=get_file_global('firmware_family'),
                                           # below line changed for SRFN setup
                                           version_string=get_file_global('firmware_version_string'))

            if not os.path.exists(os.path.join(get_file_global('project_path'), 'Support', 'Builds')):
                builds_path=os.path.join(get_file_global('project_path'), 'Support', 'Builds')
                os.makedirs(builds_path)
                set_file_global(name=builds_path, value=builds_path)
        else:
            get_fw_version(os.path.abspath(get_file_global('path_to_version_dot_h')))

            artifact_folders = ArtifactFolders(project_path=os.path.abspath(get_file_global('project_path')),
                                           release_name=get_file_global('release_name'),
                                           firmware_family=get_file_global('firmware_family'),
                                           version_string=get_file_global('version_string'))

    if (phase is None) or (phase == 1):
        builder = FirmwareBuilder()

        if int(get_file_global(name='firmware_family')) == 99:
            if(get_file_global(name='firmware_type_ep')) == 'RA6':
                builder.build_SRFN_RA6_firmware()
            else:
                builder.build_SRFN_firmware()
                # flash_SRFN_image()
        else:
            builder.build_firmware()


    if (phase is None) or (phase == 2):
        if int(get_file_global(name='firmware_family')) == 99:
            #build_xml_files(fw_family=int(get_file_global('firmware_family')),
                            # version_major=int(get_file_global("firmware_ver")),
                            # version_minor=int(get_file_global("firmware_rev")),
                            # version_build=int(get_file_global("firmware_build")),
                            # bootloader_major=int(get_file_global("bootloader_ver")),
                            # bootloader_minor=int(get_file_global("bootloader_rev")),
                            # bootloader_build=int(get_file_global("bootloader_build")),
                            # TODO: may not be able to do,  try to default out the other params
                            #  ONCE i TRACKED DOWN AND LOOKED AT THE xml WE WILL NEED TO TOTALLY REWORK IT AND ITS
                            #  CONTENTS TO BE ABLE TO USE IT (MAYBE THE BEGININGS OF A CONFIG MANAGER)
                            # appCrc=int(get_file_global("app_crc")),
                            # xml_gen_dir=os.path.abspath(get_file_global(name='xml_dir')))
                            pass
        else:
            build_xml_files(fw_family=int(get_file_global('firmware_family')),
                        version_major=int(get_file_global("fw_major")),
                        version_minor=int(get_file_global("fw_minor")),
                        version_build=int(get_file_global("fw_build")),
                        appCrc=int(get_file_global("app_crc")),
                        xml_gen_dir=os.path.abspath(get_file_global(name='xml_dir')))


    if (phase is None) or (phase == 3):
        if int(get_file_global(name='firmware_family')) == 99:
            pass
        else:
            run_pc_lint(path_to_output_dir=os.path.abspath(get_file_global(name='pc_lint_dir')))

    if (phase is None) or (phase == 4):
        if int(get_file_global(name='firmware_family')) == 99:
            pass
        else:
            build_doxygen_files(path_to_doxygen_run_from_dir=os.path.abspath(os.path.join(project_path, "support")),
                                path_to_doxygen_output_dir=os.path.abspath(get_file_global(name='doxygen_dir')))

    if (phase is None) or (phase == 5):
        if int(get_file_global(name='firmware_family')) == 99:
            pass
        else:
            # make a zip file of just the doxygen files
            doxygen_dir = os.path.abspath(get_file_global('doxygen_dir'))
            os.chdir(os.path.join(doxygen_dir, ".."))
            zip_name = "doxygen"
            # creating zip file
            shutil.make_archive(zip_name, 'zip', doxygen_dir)
            # Calling out the 2 paths needed to moved the zipped file
            doxygen_zip_path = os.path.join(os.path.dirname(doxygen_dir), zip_name + ".zip")
            new_doxygen_zip_path = os.path.join(os.environ["BUILD_ARTIFACTSTAGINGDIRECTORY"], zip_name + ".zip")
            # Moving the zipped file
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
        if int(get_file_global(name='firmware_family')) == 99:

            # creating a place for the zipped up builds, to be achieved
            if not os.path.exists("C:\\temp\\Zipped_Builds"):
                zipped_path="C:\\temp\\Zipped_Builds"
                os.makedirs(zipped_path)


            os.environ["BUILD_ARTIFACT_STAGING_DIR"] = "C:\\temp\\Zipped_Builds"


            shutil.copy(enviro_file_path,get_file_global(name="build_artifacts_dir"))

            build_dir = os.path.abspath(get_file_global(name ='build_dir'))
            os.chdir(os.path.join(build_dir, ".."))
            zip_name = get_file_global(name="build_package")
            shutil.make_archive(zip_name, 'zip', build_dir)
            to_B_zipped_path = os.path.join(os.path.dirname(build_dir), zip_name + ".zip")
            newly_built_N_zipped = os.path.join(os.environ["BUILD_ARTIFACT_STAGING_DIR"], zip_name + ".zip")
            shutil.move(to_B_zipped_path, newly_built_N_zipped)
            # TODO I need to call the clean release folders func, I'm finding leftovers in the my archives.


            print(f'##vso[task.setvariable variable=SRFN_Art]{newly_built_N_zipped}')
            # print(f'##vso[task.setvariable variable=xml_path]{xml_path}')


            # deleting the contents in the releases folder
            shutil.rmtree(path=get_file_global(name = "build_dir"))
            # rel_folder = os.path.join(get_file_global(name="project_path"),"ReleaseFiles")
            # shutil.rmtree(path=rel_folder)





        else:
            builder = FirmwareBuilder(signing_only=True)
            builder.sign_firmware()

            shutil.rmtree(path=os.path.join(os.environ["AGENT_BUILDDIRECTORY"], "stage_1_artifacts", "temp"))
            shutil.rmtree(path=os.path.join(os.environ["AGENT_BUILDDIRECTORY"], "stage_1_artifacts", "unit_test_images"))
            shutil.rmtree(path=os.path.join(os.environ["AGENT_BUILDDIRECTORY"], "stage_1_artifacts", "PC_Lint_Output"))
            shutil.rmtree(path=os.path.join(os.environ["AGENT_BUILDDIRECTORY"], "stage_1_artifacts", "build_artifacts"))
            shutil.rmtree(path=os.path.join(os.environ["AGENT_BUILDDIRECTORY"], "stage_1_artifacts", "files_to_be_signed"))
