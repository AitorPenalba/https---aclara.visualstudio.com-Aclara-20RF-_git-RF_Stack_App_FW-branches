import os
import shutil
import subprocess
import time
import parse
# import serial
from ascii_hex_utilities import ASCIIHEX_LinetoBytes, ASCIIHEX_ConvertToBinary, ASCIIHEX_WriteToFile_Addr
# from crypto import CryptoUtility
from utility import get_addr_from_fixedmemloc_h, CRC, read_challenge_info
from firmwareFlasher import flash_image
from fileGlobals import set_file_global, get_file_global
# from cryptography.hazmat.primitives.asymmetric import utils


class FirmwareBuilder:
    def __init__(self, signing_only=False):
        self.project_path = os.path.abspath(get_file_global('project_path'))
        self.firmware_family = int(get_file_global('firmware_family'))
        self.path_to_srec_cat_exe = os.path.abspath(get_file_global('path_to_srec_cat_exe'))


        if not signing_only:
            self.mtu_com_port = get_file_global('mtu_com_port')
            self.path_to_iar_build_exe = os.path.abspath(get_file_global('path_to_iar_build_exe'))

            self.release_name = get_file_global('release_name')

            self.iar_app_target_name = get_file_global(name='iar_app_target_name')

            set_file_global("release_name", self.release_name)

            self.bootloader_target_name = "Boot_Release"

            self.dfw_product_type = 0   # 0 for MTU, 1 for DCU

            self.bootloader_ewp_name = "bootloader.ewp"
            self.application_ewp_name = "application.ewp"

            set_file_global(name='path_to_fixedmemloc_dot_h', value=os.path.join(self.project_path, "fixedmemloc.h"))

            self.fw_major = int(get_file_global(name='fw_major'))
            self.fw_minor = int(get_file_global(name='fw_minor'))
            self.fw_build = int(get_file_global(name='fw_build'))
            self.version_string = get_file_global(name='version_string')

            # the following will be defined in member functions
            self.boot_crc = None
            self.app_crc = None
            self.crypto = None

    def sign_firmware(self):
        to_be_signed_dir = os.path.join(os.environ["AGENT_BUILDDIRECTORY"],
                                        "stage_1_artifacts", "files_to_be_signed")

        for filename in os.listdir(to_be_signed_dir):
            if "application" in filename:
                build_type, fw_major, fw_minor, fw_build = parse.parse("application_{}_Release_{}.{}.{}_file_to_be_signed.data", filename)
                break

        fw_version = "{:02d}.{:02d}.{:04d}".format(int(fw_major), int(fw_minor), int(fw_build))

        sig_dir = get_file_global("path_to_signatures")
        for filename in os.listdir(sig_dir):
            if "application" in filename:
                path_to_app_sig_file = os.path.join(sig_dir, filename)
            elif "bootloader" in filename:
                path_to_boot_sig_file = os.path.join(sig_dir, filename)
            else:
                raise RuntimeError('The needed signature files are not present or are named improperly (must contain "bootloader" or "application" in the filename')

        app_sig = self.load_signature_from_file(path_to_app_sig_file)
        boot_sig = self.load_signature_from_file(path_to_boot_sig_file)

        app_filename = "application_" + get_file_global(name='iar_app_target_name') + "_code.txt"
        boot_filename = "bootloader_Boot_Release_code.txt"

        app_file_from_first_run = os.path.join(os.environ["AGENT_BUILDDIRECTORY"], "stage_1_artifacts", "temp", app_filename)
        boot_file_from_first_run = os.path.join(os.environ["AGENT_BUILDDIRECTORY"], "stage_1_artifacts", "temp", boot_filename)

        # Write Signatures to Images
        print(f"Writing signature to {app_file_from_first_run}")
        ASCIIHEX_WriteToFile_Addr(app_file_from_first_run, 0x4C10, app_sig, 64)

        print(f"Writing signature to {boot_file_from_first_run}")
        ASCIIHEX_WriteToFile_Addr(boot_file_from_first_run,  0x4410, boot_sig, 64)

        # Apply the header
        tag_mtu_type = get_file_global(name='dfw_target_mtu_type')
        tag_major = get_file_global(name='dfw_target_fw_major')
        tag_minor = get_file_global(name='dfw_target_fw_minor')
        tag_build = get_file_global(name='dfw_target_fw_build')

        app_tag = '0,' + tag_mtu_type + "," + "{}.{}.{}".format(tag_major, tag_minor, tag_build) + ",0"
        boot_tag = '0,' + tag_mtu_type + "," + "0.0.0" + ",0"

        target_version = "{:02d}.{:02d}.{:04d}".format(int(tag_major), int(tag_minor), int(tag_build))
        app_img_name = f'dfw-application-{build_type}-V{fw_version}_for_V{target_version}.s37'
        boot_img_name = f'dfw-bootloader-V{fw_version}_for_V{target_version}.s37'

        ImgOutputDir = self.add_header_and_finalize_image(pathASCIIHexFile=app_file_from_first_run,
                                                          tag=app_tag,
                                                          output_name=app_img_name)

        BootOutputDir = self.add_header_and_finalize_image(pathASCIIHexFile=boot_file_from_first_run,
                                                           tag=boot_tag,
                                                           output_name=boot_img_name)

    def add_header_and_finalize_image(self, pathASCIIHexFile, tag, output_name, verbose=True):
        to_be_signed_dir = os.path.join(os.environ["AGENT_BUILDDIRECTORY"], "stage_1_artifacts")
        pathToDest = os.path.join(to_be_signed_dir, output_name)

        # execute the SREC command for the image

        functionParams = [self.path_to_srec_cat_exe,
                          "-header=" + tag,
                          pathASCIIHexFile,
                          "-Ascii_Hex", "-o",
                          pathToDest]
        process = subprocess.Popen(functionParams, stdout=subprocess.PIPE, encoding='utf_8')
        for line in iter(process.stdout.readline, ''):
            print(f" >>>>> {line.strip()}")

        # make sure that the output file was created
        if not os.path.isfile(pathToDest):
            print("Error!!!!!!!!! Creating {} failed".format(pathToDest))
            raise Exception()

        return pathToDest

    def load_signature_from_file(self, sig_file):
        """
        Load and decode the signature from a file
        :param file sig_file:
        :return: bytes signature
        """
        sig_file = sig_file.replace("\"", "")
        with open(sig_file, "rb") as f_sig:
            signature = f_sig.read()
            # Decode signature to r, s value
            r, s = utils.decode_dss_signature(signature)
            signature_decoded_bytes = bytes(bytearray().fromhex('{0:064X}'.format(r) + '{0:064X}'.format(s)))
            return signature_decoded_bytes


    def build_firmware(self):
        self.clean_release_folders()

        available_unit_tests = self.get_available_unit_tests_from_unittest_h()

        for unit_test_id in available_unit_tests:
            self.update_unittest_h(unit_test_to_enable=unit_test_id)
            unit_test_target_name = self.iar_app_target_name + "_Unit_Tests"
            warning_count, error_count, unit_test_s37_path = self.build_fw_image(ewp_name=self.application_ewp_name,
                                                                                 target_name=unit_test_target_name,
                                                                                 make_archive=False)

            if (error_count > 0):
                print(f"Building {unit_test_target_name} failed ({error_count} errors and {warning_count} warnings)")
                print('##vso[task.complete result=Failed;]')
                return

            shutil.move(unit_test_s37_path,
                        os.path.join(os.path.abspath(get_file_global(name='unit_test_dir')), unit_test_id + ".s37"))

        warning_count, error_count, app_s37_path = self.build_fw_image(ewp_name=self.application_ewp_name,
                                                                       target_name=self.iar_app_target_name)

        if (warning_count + error_count) > 0:
            print(f"Building the application image failed with {error_count} errors and {warning_count} warnings ")
            print('##vso[task.complete result=Failed;]')
            return

        warning_count, error_count, boot_s37_path = self.build_fw_image(ewp_name=self.bootloader_ewp_name,
                                                                        target_name=self.bootloader_target_name)

        if (warning_count + error_count) > 0:
            print(f"Building the bootloader image failed with {error_count} errors and {warning_count} warnings ")
            print('##vso[task.complete result=Failed;]')
            return

        path_to_app_dest, path_to_bootloader_dest = self.get_codespace_from_s37(app_build_file=app_s37_path,
                                                                                boot_build_file=boot_s37_path)

        self.boot_crc = self.calculate_and_append_crc_to_image(path_to_file=path_to_bootloader_dest,
                                                               start_addr=get_addr_from_fixedmemloc_h("BOOT_CRC_START"),
                                                               end_addr=get_addr_from_fixedmemloc_h("BOOT_CRC_LOC"),
                                                               wrt_addr=get_addr_from_fixedmemloc_h("BOOT_CRC_LOC"))

        self.app_crc = self.calculate_and_append_crc_to_image(path_to_file=path_to_app_dest,
                                                              start_addr=get_addr_from_fixedmemloc_h("APP_CRC_START"),
                                                              end_addr=get_addr_from_fixedmemloc_h("APP_CRC_LOC"),
                                                              wrt_addr=get_addr_from_fixedmemloc_h("APP_CRC_LOC"))

        set_file_global("app_crc", self.app_crc)

        self.gen_mfg_image(app_build_file=app_s37_path, bootloader_build_file=boot_s37_path)

        self.save_backup_of_cryptokeygen_h()

        self.update_cryptokeygen_h()

        warning_count, error_count, app_s37_path = self.build_fw_image(ewp_name=self.application_ewp_name,
                                                                       target_name="CryptoKeyGenerator")

        self.restore_backup_of_cryptokeygen_h()

        flash_image(path_to_image=app_s37_path)

        time.sleep(10)

        self.generate_challenge_response_data()

        self.crypto.encrypt_image(path_in=path_to_bootloader_dest,
                                  enc_start_addr=get_addr_from_fixedmemloc_h("BOOT_AES_START"),
                                  enc_end_addr=get_addr_from_fixedmemloc_h("BOOT_AES_END") + 2,
                                  path_out=None)

        self.crypto.encrypt_image(path_in=path_to_app_dest,
                                  enc_start_addr=get_addr_from_fixedmemloc_h("APP_AES_START"),
                                  enc_end_addr=get_addr_from_fixedmemloc_h("APP_AES_END") + 2,
                                  path_out=None)

        shutil.copy(path_to_bootloader_dest, path_to_bootloader_dest + "rightBeforePrepareForSigning.txt")
        shutil.copy(path_to_app_dest, path_to_app_dest + "rightBeforePrepareForSigning.txt")

        output_file_path = os.path.join(os.path.abspath(get_file_global(name='files_to_be_signed')), "bootloader_" +
                                        self.iar_app_target_name + "_" + self.version_string +
                                        "_file_to_be_signed.data")
        self.prepare_image_for_signing(image_path=path_to_bootloader_dest,
                                       code_start_addr=get_addr_from_fixedmemloc_h("BOOT_CRC_START"),
                                       code_end_addr=get_addr_from_fixedmemloc_h("BOOT_CRC_LOC") + 2,
                                       hash_addr=get_addr_from_fixedmemloc_h("BOOT_HASH_LOC"),
                                       output_file_path=output_file_path)

        output_file_path = os.path.join(os.path.abspath(get_file_global(name='files_to_be_signed')), "application_" +
                                        self.iar_app_target_name + "_" + self.version_string +
                                        "_file_to_be_signed.data")
        self.prepare_image_for_signing(image_path=path_to_app_dest,
                                       code_start_addr=get_addr_from_fixedmemloc_h("APP_CRC_START"),
                                       code_end_addr=get_addr_from_fixedmemloc_h("APP_CRC_LOC") + 2,
                                       hash_addr=get_addr_from_fixedmemloc_h("APP_HASH_LOC"),
                                       output_file_path=output_file_path)

    def clean_release_folders(self):

        dir_to_delete = os.path.join(self.project_path, self.bootloader_target_name)
        if os.path.exists(dir_to_delete):
            shutil.rmtree(dir_to_delete)

        dir_to_delete = os.path.join(self.project_path, self.iar_app_target_name)
        if os.path.exists(dir_to_delete):
            shutil.rmtree(dir_to_delete)

    def build_fw_image(self, ewp_name, target_name, make_archive=True):
        print(f"Building the {target_name} target in the {ewp_name} project")
        log_file_name = f"{ewp_name.replace('.ewp', '')}-{target_name}-IarBuild.log"
        log_file_path = os.path.join(os.path.abspath(get_file_global(name='build_artifacts_dir')), log_file_name)

        print(f"Logging to {log_file_path}")

        ewp_path = os.path.join(self.project_path, ewp_name)

        with open(log_file_path, 'w') as f:
            functionParams = [self.path_to_iar_build_exe, ewp_path, "-build", target_name, '-log', 'all', '-parallel', '10']

            process = subprocess.Popen(functionParams, stdout=subprocess.PIPE, encoding='utf_8')

            for line in iter(process.stdout.readline, ''):
                f.write(line)
                print(f"{ewp_name}_{target_name} >>>>> {line.strip()}")

                if "Total number of errors" in line:
                    error_count = int(parse.parse("Total number of errors: {}", line.strip())[0])

                if "Total number of warnings" in line:
                    warning_count = int(parse.parse("Total number of warnings: {}", line.strip())[0])

        # copy output files to the artifacts directory
        exe_output_dir = os.path.join(os.path.abspath(self.project_path), target_name)

        if make_archive:
            prev_working_dir = os.getcwd()

            run_from_path = os.path.join(os.path.abspath(self.project_path), "BuildServer")
            os.chdir(run_from_path)

            base_name = ewp_name.replace('.ewp', '') + "_" + target_name

            shutil.make_archive(base_name=base_name,
                                format="zip",
                                root_dir=exe_output_dir)

            os.chdir(prev_working_dir)

            zip_from_path = os.path.join(run_from_path, base_name + ".zip")
            zip_to_path = os.path.join(os.path.abspath(get_file_global(name='build_artifacts_dir')), base_name + ".zip")
            shutil.move(zip_from_path, zip_to_path)

        d43_from_path = os.path.join(exe_output_dir, "Exe", ewp_name.replace('.ewp', '') + ".d43")
        d43_working_to_path = os.path.join(os.path.abspath(get_file_global(name='temp_dir')),
                                           ewp_name.replace('.ewp', '') + "_" + target_name + ".d43")
        d43_archive_to_path = os.path.join(os.path.abspath(get_file_global(name='build_artifacts_dir')),
                                           ewp_name.replace('.ewp', '') + "_" + target_name + "_archive1.d43")
        if os.path.exists(d43_from_path):
            shutil.copyfile(d43_from_path, d43_working_to_path)
            shutil.copyfile(d43_from_path, d43_archive_to_path)

        s37_from_path = os.path.join(exe_output_dir, "Exe", ewp_name.replace('.ewp', '') + ".s37")
        s37_working_to_path = os.path.join(os.path.abspath(get_file_global(name='temp_dir')),
                                           ewp_name.replace('.ewp', '') + "_" + target_name + ".s37")
        s37_archive_to_path = os.path.join(os.path.abspath(get_file_global(name='build_artifacts_dir')),
                                           ewp_name.replace('.ewp', '') + "_" + target_name + "_archive1.s37")
        if os.path.exists(s37_from_path):
            shutil.copyfile(s37_from_path, s37_working_to_path)
            shutil.copyfile(s37_from_path, s37_archive_to_path)

        return warning_count, error_count, s37_working_to_path

    def get_codespace_from_s37(self, app_build_file=None, boot_build_file=None):
        """
        Manipulate and convert the image

        :param file app_build_file:
        :param file boot_build_file:

        :return:
        """

        # execute the SREC command for the application image and bootloader
        logFileName = "GetCodespaceFromS37.log"
        logFilePath = os.path.join(os.path.abspath(get_file_global(name='build_artifacts_dir')), logFileName)

        with open(logFilePath, 'w') as f:
            # application
            pathToAppDest = app_build_file.replace(".s37", "_code.txt")
            functionParams = [self.path_to_srec_cat_exe,
                              app_build_file,
                              "-crop",
                              hex(get_addr_from_fixedmemloc_h("APP_MEM_START")),
                              hex(get_addr_from_fixedmemloc_h("APP_CRC_LOC")),
                              "-fill",
                              "0xff",
                              hex(get_addr_from_fixedmemloc_h("APP_MEM_START")),
                              hex(get_addr_from_fixedmemloc_h("APP_CRC_LOC")),
                              "-o",
                              pathToAppDest,
                              "-Ascii_Hex"]

            process = subprocess.Popen(functionParams, stdout=subprocess.PIPE, encoding='utf_8')
            for line in iter(process.stdout.readline, ''):
                f.write(line)
                print("srec_app >>>>> ", line.strip())

            # bootloader
            pathToBootloaderDest = boot_build_file.replace(".s37", "_code.txt")
            functionParams = [self.path_to_srec_cat_exe,
                              boot_build_file,
                              "-Motorola",
                              "-crop",
                              hex(get_addr_from_fixedmemloc_h("BOOT_MEM_START")),
                              hex(get_addr_from_fixedmemloc_h("BOOT_RESET_VECTOR")),
                              boot_build_file,
                              "-Motorola",
                              "-crop",
                              "0xFFFE",
                              "0x10000",
                              "-offset",
                              "-0xFFFE",
                              "-offset",
                              hex(get_addr_from_fixedmemloc_h("BOOT_RESET_VECTOR")),
                              boot_build_file,
                              "-Motorola",
                              "-crop",
                              hex(get_addr_from_fixedmemloc_h("BOOT_RESET_VECTOR") + 2),
                              hex(get_addr_from_fixedmemloc_h("BOOT_CRC_LOC")),
                              "-fill",
                              "0xff",
                              hex(get_addr_from_fixedmemloc_h("BOOT_RESET_VECTOR") + 2),
                              hex(get_addr_from_fixedmemloc_h("BOOT_CRC_LOC")),
                              "-o",
                              pathToBootloaderDest,
                              "-Ascii_Hex"]
            process = subprocess.Popen(functionParams, stdout=subprocess.PIPE, encoding='utf_8')
            for line in iter(process.stdout.readline, ''):
                f.write(line)
                print("srec_bootloader >>>>> ", line.strip())

        # make sure that the output files were created
        if not os.path.isfile(pathToAppDest):
            print("Error!!!!!!!!! Creating {} failed".format(pathToAppDest))
            print("\t {} was not found".format(pathToAppDest))
            raise Exception()

        if not os.path.isfile(pathToBootloaderDest):
            print("Error!!!!!!!!! Creating {} failed".format(pathToBootloaderDest))
            print("\t {} was not found".format(pathToBootloaderDest))
            raise Exception()

        return pathToAppDest, pathToBootloaderDest

    def calculate_and_append_crc_to_image(self, path_to_file, start_addr, end_addr, wrt_addr):
        """
        Calculate ASCIIHEX format image CRC value

        :param file path_to_file: ASCIIHEX image file
        :param int start_addr: CRC calculation storing address
        :param int end_addr: The end address is the next byte of the last CRC calculate byte
        :param int wrt_addr: [Optional] if set, the 2 bytes CRC code will be write to image in Big-endian order

        :return: int CRC value
        """
        _pathToTemp = os.path.dirname(os.path.abspath(path_to_file)) + '\\crc_temp.txt'
        _cal_len = end_addr - start_addr

        functionParams = [self.path_to_srec_cat_exe,
                          path_to_file,
                          "-ASCIIHEX",
                          "-crop",
                          hex(start_addr),
                          hex(end_addr),
                          "-offset",
                          '-' + hex(start_addr),
                          "-fill",
                          "0xff",
                          hex(0),
                          hex(_cal_len + 16 - _cal_len % 16),  # _ASCIIHEX_Line_bytes = 16
                          "-o",
                          _pathToTemp,
                          "-Ascii_Hex"]
        process = subprocess.Popen(functionParams, stdout=subprocess.PIPE, encoding='utf_8')
        for line in iter(process.stdout.readline, ''):
            print(f"srec_crc >>>>> {line.strip}")

        with open(_pathToTemp, 'r') as fr:
            lines = fr.readlines()
            if len(lines) < 2:
                raise Exception('File does not contain data.')

        # CRC
        cal_crc = CRC()
        # Loop line[1:-2], skip the 1st and the last two lines
        _cntData = 0
        for idx in range(1, len(lines) - 2, 1):
            # Convert line format to bytes
            data = ASCIIHEX_LinetoBytes(lines[idx])

            _crc_inline_idx = 0
            while (_cntData < _cal_len) and (_crc_inline_idx < len(data)):
                _crc_inline_idx = _crc_inline_idx + 1
                _cntData = _cntData + 1

            # Cal CRC Here
            if (_cntData <= _cal_len) and (_crc_inline_idx <= len(data)):
                cal_crc.update(data[:_crc_inline_idx])

        if wrt_addr is not None:
            functionParams = [self.path_to_srec_cat_exe,
                              path_to_file,
                              "-ASCIIHEX",
                              "-exclude",
                              hex(wrt_addr),
                              hex(wrt_addr + 2),
                              "-generate",
                              hex(wrt_addr),
                              hex(wrt_addr + 2),
                              "-constant-b-e",
                              str(cal_crc.result),
                              '2',
                              "-o",
                              path_to_file,
                              "-Ascii_Hex"]

            process = subprocess.Popen(functionParams, stdout=subprocess.PIPE, encoding='utf_8')
            for line in iter(process.stdout.readline, ''):
                print(f"srec >>>>> {line.strip()}")

        archive_path = os.path.join(os.path.abspath(get_file_global(name='build_artifacts_dir')),
                                    os.path.basename(path_to_file).replace(".txt", "_archive2.txt"))
        print(f"Copying backup from {path_to_file} to {archive_path}")
        shutil.copy(path_to_file, archive_path)

        return cal_crc.result

    def gen_mfg_image(self, app_build_file, bootloader_build_file):
        crc_str = "{:04X}".format(self.app_crc)
        mfg_img = f"mfg-{self.release_name}-fwFamily{self.firmware_family}-V{self.version_string}-Crc0x{crc_str}.s37"

        logFileName = "GetCodespaceFromS37.log"
        logFilePath = os.path.join(os.path.abspath(get_file_global(name='build_artifacts_dir')), logFileName)
        with open(logFilePath, 'w') as f:
            # MFG
            pathToMFGDest = os.path.join(os.path.abspath(get_file_global(name='build_dir')), mfg_img)
            functionParams = [self.path_to_srec_cat_exe,
                              "−disable=data-count",
                              bootloader_build_file,
                              "-Motorola",
                              "-fill",
                              "0xff",
                              hex(get_addr_from_fixedmemloc_h("BOOT_MEM_START")),
                              hex(get_addr_from_fixedmemloc_h("BOOT_CRC_LOC") + 2),
                              app_build_file,
                              "-Motorola",
                              "-fill",
                              "0xff",
                              hex(get_addr_from_fixedmemloc_h("APP_MEM_START")),
                              hex(get_addr_from_fixedmemloc_h("APP_CRC_LOC") + 2),
                              "-exclude",
                              "0xFFFE",
                              "0x10000",
                              "-o",
                              pathToMFGDest]

            process = subprocess.Popen(functionParams, stdout=subprocess.PIPE, encoding='utf_8')
            for line in iter(process.stdout.readline, ''):
                f.write(line)
                print(f"gen_mfg_image >>>>> {line.strip()}")

        print(f"Mfg Image Created: {pathToMFGDest}")

        set_file_global("mfg_image_path", pathToMFGDest)

        return pathToMFGDest

    def save_backup_of_cryptokeygen_h(self):
        path_to_cryptoKeyGenDotH = os.path.join(self.project_path, "cryptoKeyGen.h")

        if os.path.exists(path_to_cryptoKeyGenDotH + ".backup"):
            os.remove(path_to_cryptoKeyGenDotH + ".backup")

        os.rename(path_to_cryptoKeyGenDotH, path_to_cryptoKeyGenDotH + ".backup")

    def restore_backup_of_cryptokeygen_h(self):
        path_to_cryptoKeyGenDotH = os.path.join(self.project_path, "cryptoKeyGen.h")

        if os.path.exists(path_to_cryptoKeyGenDotH):
            os.remove(path_to_cryptoKeyGenDotH)

        os.rename(path_to_cryptoKeyGenDotH + ".backup", path_to_cryptoKeyGenDotH)

    def update_cryptokeygen_h(self):
        path_to_cryptoKeyGenDotH = os.path.join(self.project_path, "cryptoKeyGen.h")

        with open(path_to_cryptoKeyGenDotH, "w") as file:
            file.write("#ifndef CRYPTO_KEYGEN_H\n")
            file.write("#define CRYPTO_KEYGEN_H\n\n")
            file.write("#if CRYPTO_KEY_GEN==1\n\n")

            file.write(f"#define DFW_CHALLENGE_FW_REV_MAJOR  {self.fw_major}\n")
            file.write(f"#define DFW_CHALLENGE_FW_REV_MINOR  {self.fw_minor}\n")
            file.write(f"#define DFW_CHALLENGE_FW_REV_BUILD  {self.fw_build}\n\n")

            if self.firmware_family == 0:
                file.write("#define DFW_CHALLENGE_FW_FAMILY   S3X5X_NORMAL_GAS_FW_FAMILY_ID\n\n")
            elif self.firmware_family == 1:
                file.write("#define DFW_CHALLENGE_FW_FAMILY   S3X5X_PULSE_GAS_FW_FAMILY_ID\n\n")
            elif self.firmware_family == 2:
                file.write("#define DFW_CHALLENGE_FW_FAMILY   S3X5X_ENCODER_AND_PULSE_WATER_FW_FAMILY_ID\n\n")
            elif self.firmware_family == 3:
                file.write("#define DFW_CHALLENGE_FW_FAMILY   S3X5X_ECODER_AND_DASHX_WATER_FW_FAMILY_ID\n\n")
            else:
                raise RuntimeError("I don't know how to handle that fw_family")

            file.write("#define DFW_CHALLENGE_PRODUCT_TYPE  0\n\n")

            file.write("void KeyGen_GenerateChallengeResponse(void);\n\n")

            file.write("#endif // CRYPTO_KEY_GEN==1\n\n")

            file.write("#endif  //_CRYPTO_KEYGEN_H_\n")

    def update_unittest_h(self, unit_test_to_enable):
        path_to_unitTestDotH = os.path.join(self.project_path, "General", "UnitTest.h")

        with open(path_to_unitTestDotH, "r") as file:
            file_contents = file.read()

        file_lines = file_contents.split("\n")
        file_contents = ""

        start_delimiter_found = False
        end_delimiter_found = False
        for file_line in file_lines:
            if not start_delimiter_found:
                if "BUILD_SERVER_SECTION_START" in file_line:
                    start_delimiter_found = True
                    file_contents += (file_line + "\n")
                else:
                    file_contents += (file_line + "\n")
            elif "BUILD_SERVER_SECTION_END" in file_line:
                file_contents += (file_line + "\n")
                end_delimiter_found = True
            elif end_delimiter_found:
                file_contents += (file_line + "\n")
            else:
                _junk, unit_test_name, _junk2 = parse.parse("{}define {} {}", file_line)

                if unit_test_name == unit_test_to_enable:
                    file_contents += f"#define {unit_test_name} 1\n"
                else:
                    file_contents += f"#define {unit_test_name} 0\n"

        with open(path_to_unitTestDotH, "w") as file:
            file.write(file_contents)

    def get_available_unit_tests_from_unittest_h(self):
        path_to_unitTestDotH = os.path.join(self.project_path, "General", "UnitTest.h")

        with open(path_to_unitTestDotH, "r") as file:
            file_contents = file.read()

        file_lines = file_contents.split("\n")

        unit_test_names = []

        start_delimiter_found = False
        for file_line in file_lines:
            if not start_delimiter_found:
                if "BUILD_SERVER_SECTION_START" in file_line:
                    start_delimiter_found = True
                else:
                    continue
            elif "BUILD_SERVER_SECTION_END" in file_line:
                break
            else:
                _junk, unit_test_name, _junk2 = parse.parse("{}define {} {}", file_line)
                unit_test_names.append(unit_test_name)

        return unit_test_names

    def generate_challenge_response_data(self):
        uart = serial.Serial(port=self.mtu_com_port, baudrate=1200, timeout=3, stopbits=2)

        challengeInfo = read_challenge_info(uart)
        dfwChallengeResponse = [int(x) for x in challengeInfo[0:32]]
        dfwChallengeFwRevMajor = int(challengeInfo[32])
        dfwChallengeFwRevMinor = int(challengeInfo[33])
        dfwChallengeFwRevBuildMsb = int(challengeInfo[34])
        dfwChallengeFwRevBuildLsb = int(challengeInfo[35])
        dfwChallengeFwFamily = int(challengeInfo[36])
        dfwChallengeProductType = int(challengeInfo[37])

        dfwChallengeFwBuild = (dfwChallengeFwRevBuildMsb << 8) + dfwChallengeFwRevBuildLsb

        file_object = open(os.path.join(os.path.abspath(get_file_global(name='build_artifacts_dir')),
                                        "challenge_response_file.txt"), "w")
        file_object.write(f"dfwChallengeFwRevMajor = {dfwChallengeFwRevMajor}\n")
        file_object.write(f"dfwChallengeFwRevMinor = {dfwChallengeFwRevMinor}\n")
        file_object.write(f"dfwChallengeFwBuild = {dfwChallengeFwBuild}\n")
        file_object.write(f"dfwChallengeFwFamily = {dfwChallengeFwFamily}\n")
        file_object.write(f"dfwChallengeProductType = {dfwChallengeProductType}\n")
        file_object.write(f"dfwChallengeResponse = {dfwChallengeResponse}\n")
        file_object.close()

        if ((self.fw_major != dfwChallengeFwRevMajor) or
                (self.fw_minor != dfwChallengeFwRevMinor) or
                (self.fw_build != dfwChallengeFwBuild) or
                (self.firmware_family != dfwChallengeFwFamily) or
                (self.dfw_product_type != dfwChallengeProductType)):

            raise RuntimeError("Error! The ver/prod from the challenge response file does not match the build config!")

        dfw_challenge_response = bytearray(dfwChallengeResponse)

        self.crypto = CryptoUtility(dfw_challenge_response,
                                    fw_major=self.fw_major,
                                    fw_minor=self.fw_minor,
                                    fw_build=self.fw_build,
                                    firmware_family=self.firmware_family,
                                    dfw_product_type=self.dfw_product_type)

    def prepare_image_for_signing(self, image_path, code_start_addr, code_end_addr, hash_addr, output_file_path):
        # Calculate Hash
        self.crypto.calculate_image_hash(path_to_image=image_path,
                                         start_addr=code_start_addr,
                                         end_addr=code_end_addr,
                                         write_to_addr=hash_addr)

        print(f"in prepare image for signing: {image_path}")

        # The HSM signing command includes SHA256 and ECDSA, so generate a binary image file for HSM signing
        ASCIIHEX_ConvertToBinary(image_path, code_start_addr, code_end_addr, output_file_path)


if __name__ == "__main__":
    from fileGlobals import init_file_globals, set_file_global, get_file_global
    from utility import ArtifactFolders, get_fw_version

    enviro_file_path = "C:\\temp\\build_server_environment_variables.txt"
    if os.path.exists(enviro_file_path):
        os.remove(enviro_file_path)

    os.environ["environ_path"] = "C:\\temp\\build_server_environment_variables.txt"
    init_file_globals()

    set_file_global(name='root_dir', value=os.getcwd())
    set_file_global(name='project_path', value="C:\\Development\\STAR\\MTU\\Gen3_Products\\GasAndWaterMtus2017\\Main")
    set_file_global(name='path_to_version_dot_h', value="C:\\Development\\STAR\\MTU\\Gen3_Products\\GasAndWaterMtus2017\\Main\\Encoder_Version.h")
    set_file_global(name='firmware_family', value="2")
    set_file_global(name='mtu_com_port', value="COM5")
    set_file_global(name='path_to_uniflash_batch_file', value="C:\\ti\\uniflash_windows_CLI\\dslite.bat")
    set_file_global(name='path_to_uniflash_ccxml', value="C:\\ti\\uniflash_windows_CLI\\MSP430FR5964.ccxml")
    set_file_global(name='path_to_iar_build_exe',
                    value="C:\\Program Files (x86)\\IAR Systems\\Embedded Workbench 8.4\\common\\bin\\IarBuild.exe")
    set_file_global(name='path_to_srec_cat_exe', value="C:\\Development\\Tools\\srecord\\srec_cat.exe")
    set_file_global(name='path_to_doxygen_exe', value="C:\\Program Files\\doxygen\\bin\\doxygen.exe")
    set_file_global(name='path_to_pclint', value="C:\\PCLint\\pclp64.exe")
    set_file_global(name='release_name', value="Encoder_Release")

    project_path = os.path.abspath(get_file_global(name='project_path'))
    get_fw_version(os.path.abspath(get_file_global('path_to_version_dot_h')))

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

    builder = FirmwareBuilder()
    unit_test_names = builder.get_available_unit_tests_from_unittest_h()

    for unit_test_name in unit_test_names:
        builder.update_unittest_h(unit_test_to_enable=unit_test_name)
