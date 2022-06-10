import parse
import os
import time
from fileGlobals import get_file_global, set_file_global


def get_addr_from_fixedmemloc_h(location_name):
    """
    Read memory address from the Header File

    :return: None
    """
    addr = None

    file_obj = open(os.path.abspath(get_file_global('path_to_fixedmemloc_dot_h')), 'r')

    instances_found = 0
    while True:
        line = file_obj.readline()

        if location_name in line:
            instances_found += 1
            _loc_name, addr = parse.parse("#define {} {}\n", line)

            if "0x" in addr:
                addr = int(addr, 16)
            else:
                addr = int(addr)

        if line == "":
            break

    if instances_found < 1:
        raise RuntimeError(f"fixedmemloc.h does not have an instance of {location_name}")
    elif instances_found > 1:
        raise RuntimeError(f"fixedmemloc.h contains more than one instance of {location_name}")

    return addr


def get_fw_version(path_to_version_dot_h):
    """
    Read FW version from the Header File

    :return: None
    """

    ifDefString = "if defined(RELEASE_BUILD)"
    fileAsString = open(path_to_version_dot_c, 'r').read()

    print(f"Reading {path_to_version_dot_c} for version info...")

    _before, _ifdef, after = fileAsString.partition(ifDefString)

    if after == "":
        raise Exception("The format of the version.h file is wrong!")

    _before, _tag, after = after.partition("MTU_REV_MAJOR")
    major, _tag, after = after.partition("\n")
    fw_major = int(major.strip())
    print(f"fw_major={fw_major}")

    _before, _tag, after = after.partition("MTU_REV_MINOR")
    minor, _tag, after = after.partition("\n")
    fw_minor = int(minor.strip())
    print(f"fw_minor={fw_minor}")

    _before, _tag, after = after.partition("MTU_REV_RESERVED")
    _reserved, _tag, after = after.partition("\n")
    _reserved = int(_reserved.strip())

    _before, _tag, after = after.partition("MTU_BLD")
    build, _tag, after = after.partition("\n")
    fw_build = int(build.strip())
    print(f"fw_build={fw_build}")

    set_file_global(name='fw_major', value=fw_major)
    set_file_global(name='fw_minor', value=fw_minor)
    set_file_global(name='fw_build', value=fw_build)

    version_string = "%02d.%02d.%04d" % (fw_major, fw_minor, fw_build)
    set_file_global(name='version_string', value=version_string)


class CRC:
    _init_val = 0xffff
    _rncrctab = [0x0000, 0x1081, 0x2102, 0x3183, 0x4204, 0x5285, 0x6306, 0x7387,
                 0x8408, 0x9489, 0xA50A, 0xB58B, 0xC60C, 0xD68D, 0xE70E, 0xF78F]

    def _crc_gen(self, accum, ch):
        return 0xffff & ((accum >> 4) ^ self._rncrctab[(accum ^ ch) & 0x0F])

    def __init__(self):
        self.result = self._init_val

    def update(self, data):
        if isinstance(data, bytes):
            data = list(data)
        cnt = 0
        while cnt < len(data):
            val = data[cnt]
            self.result = self._crc_gen(self.result, val & 0x0F)
            self.result = self._crc_gen(self.result, val >> 4)
            cnt += 1
        return self.result


class RetryError(Exception):
    pass


class ArtifactFolders:
    def __init__(self, project_path, release_name, firmware_family, version_string):
        """
        Create the folder structure to store the various files created as part of the build process

        ReleaseFiles
            <buildName_fwFamily#_fwVersion_datetime>        # get_file_global(name='build_dir')
                build_artifacts
                buildLogs
                PC_Lint_Output
                doxygen
                xmls
                files_to_be_signed
                temp
                unit_test_images
        """

        releaseFiles_path = os.path.join(project_path, "ReleaseFiles")
        if not os.path.exists(releaseFiles_path):
            set_file_global(name='releaseFiles_path', value=releaseFiles_path)
            os.makedirs(releaseFiles_path)

        current_datetime_str = time.strftime("%Y-%m-%d-%H-%M-%S")

        build_dir_name = f"{release_name}_fwFamily{firmware_family}_{version_string}_{current_datetime_str}"
        self.build_dir = os.path.join(releaseFiles_path, build_dir_name)
        print(f"Creating build directories: {self.build_dir}")
        set_file_global(name='build_dir', value=self.build_dir)
        os.makedirs(self.build_dir)

        self.build_artifacts_dir = os.path.join(self.build_dir, "build_artifacts")
        set_file_global(name='build_artifacts_dir', value=self.build_artifacts_dir)
        os.makedirs(self.build_artifacts_dir)

        self.pc_lint_dir = os.path.join(self.build_dir, "PC_Lint_Output")
        set_file_global(name='pc_lint_dir', value=self.pc_lint_dir)
        os.makedirs(self.pc_lint_dir)

        self.doxygen_dir = os.path.join(self.build_dir, "doxygen")
        set_file_global(name='doxygen_dir', value=self.doxygen_dir)
        os.makedirs(self.doxygen_dir)

        self.xml_dir = os.path.join(self.build_dir, "xmls")
        set_file_global(name='xml_dir', value=self.xml_dir)
        os.makedirs(os.path.abspath(get_file_global(name='xml_dir')))

        self.files_to_be_signed = os.path.join(self.build_dir, "files_to_be_signed")
        set_file_global(name='files_to_be_signed', value=self.files_to_be_signed)
        os.makedirs(self.files_to_be_signed)

        self.temp_dir = os.path.join(self.build_dir, "temp")
        set_file_global(name='temp_dir', value=self.temp_dir)
        os.makedirs(self.temp_dir)

        self.unit_test_dir = os.path.join(self.build_dir, "unit_test_images")
        set_file_global(name='unit_test_dir', value=self.unit_test_dir)
        os.makedirs(self.unit_test_dir)


def read_challenge_info(uart):
    COIL_CMD_START_DELIMITER = 0x25
    COIL_CMD_READ = 0x80
    # COIL_CMD_WRITE = 0x81
    # COIL_CMD_OPREQ = 0xFE
    # COIL_CMD_ACK = 0x06
    # COIL_CMD_NACK = 0x15

    startLocation = 896
    bytesToRead = (32 + 1 + 1 + 2 + 1 + 1)
    maxAttempts = 5

    for retryCounter in range(maxAttempts):
        message = []
        try:
            message.append(COIL_CMD_START_DELIMITER)

            # command
            temp = startLocation >> 8
            temp = temp << 1
            temp |= COIL_CMD_READ
            message.append(temp)

            # address
            message.append(startLocation & 0xff)
            message.append(bytesToRead)

            # checksum = 0xff - (sum(message) & 0xff) + 1
            checksum = 0xff & (0xff - sum(message) + 1)
            message.append(checksum)

            # flush the port
            bytesWaiting = uart.inWaiting()
            if bytesWaiting > 0:
                _readData = uart.read(bytesWaiting)
                uart.port.flushInput()
                time.sleep(0.1)

                uart.write(message)
                time.sleep(0.1)

            # write the data
            uart.write(message)
            time.sleep(0.1)

            # read back the echoed data
            readBack = uart.read(len(message))
            if len(readBack) != len(message):
                raise RetryError("MTU did not properly echo back the coil data!")

            for i, writeByte in enumerate(message):
                readByte = readBack[i]
                if readByte != writeByte:
                    raise RetryError("Coil read data doesn't match data written!")

            # read back the response
            readString = uart.read(bytesToRead + 2)

            # check the CRC
            crcCalculator = CRC()
            crcCalculator.update(readString[:-2])
            calculatedCrc = crcCalculator.result

            packetCrc = (readString[-1] << 8) + readString[-2]

            if calculatedCrc != packetCrc:
                raise RetryError("CRC failure on response!")

            # gather the response and format it appropriately
            readString = readString[:-2]
            if len(readString) != bytesToRead:
                raise RetryError("Num of bytes read isn't equal num bytes requested!")

            returnData = [int(x) for x in readString]

            return returnData

        except RetryError:
            if retryCounter < maxAttempts:
                print("There was an error reading from the MTU; trying again...")
                time.sleep(.25)
            else:
                print("Giving up...")
