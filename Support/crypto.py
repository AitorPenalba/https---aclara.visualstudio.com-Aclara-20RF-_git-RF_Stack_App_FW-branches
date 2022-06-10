import os
import subprocess
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from ascii_hex_utilities import ASCIIHEX_LinetoBytes, ASCIIHEX_BytestoLine, ASCIIHEX_WriteToFile_Addr
from utility import get_addr_from_fixedmemloc_h, get_file_global


class CryptoUtility(object):
    signature_length = 64
    hash_length = 32

    def __init__(self, challenge_response, fw_major, fw_minor, fw_build, firmware_family, dfw_product_type):
        self._nonce = bytes(bytearray.fromhex('{0:02X}'.format(fw_major))) \
                      + bytes(bytearray.fromhex('{0:02X}'.format(fw_minor))) \
                      + bytes(bytearray.fromhex('{0:04X}'.format(fw_build))) \
                      + bytes(bytearray.fromhex('{0:02X}'.format(firmware_family))) \
                      + bytes(bytearray.fromhex('{0:02X}'.format(dfw_product_type))) \
                      + b'\xff\xff\xff\xff\xff\xff'

        self._cont = 0
        self.aclaraFwKey = bytearray.fromhex('7D A1 24 56 76 8B F6 B1 6A 84 F9 C7 FA C5 44 43 A9 14 71 54 8A 92 88 39 \
                                              7F 6B 49 2E F5 DC C8 9D')
        self.iv_length = 16

        self.iv = self._nonce + bytes(bytearray.fromhex('{0:08X}'.format(self._cont)))

        hasher = hashes.Hash(hashes.SHA256(), default_backend())

        hasher.update(bytes(self.aclaraFwKey))
        hasher.update(bytes(challenge_response))
        self.key = hasher.finalize()

        self.priKeyPath = "ECDSA_KeyPair_test.pem"

    def encrypt_image(self, path_in, enc_start_addr, enc_end_addr, path_out=None):
        _pathToTemp = os.path.dirname(os.path.abspath(path_in)) + '\\enc_temp.txt'

        functionParams = [os.path.abspath(get_file_global('path_to_srec_cat_exe')),
                          path_in,
                          "-ASCIIHEX",
                          "-crop",
                          hex(enc_start_addr),
                          hex(enc_end_addr),
                          "-fill",
                          "0xff",
                          hex(enc_start_addr),
                          hex(enc_end_addr),
                          "-o",
                          _pathToTemp,
                          "-ASCIIHEX"]
        process = subprocess.Popen(functionParams, stdout=subprocess.PIPE, encoding='utf_8')
        for line in iter(process.stdout.readline, ''):
            print(f"encrypt>>>>>{line}")

        with open(_pathToTemp, 'r') as f:
            lines = f.readlines()
        if path_out is None:
            path_out = path_in

        backend = default_backend()
        cipher = Cipher(algorithms.AES(self.key), modes.CTR(self.iv), backend=backend)
        encryptor = cipher.encryptor()

        idx = 0

        # Revise line[1:], skip the 1st lines
        for idx in range(1, len(lines), 1):
            # Convert line format to bytes
            data = ASCIIHEX_LinetoBytes(lines[idx])
            if data != b'':
                data = encryptor.update(data)
                # Bytes convert to Line format
                data = ASCIIHEX_BytestoLine(data)
                # Write data to line
                lines[idx] = data
            else:
                break

        encryptor.finalize()
        with open(_pathToTemp, 'w') as f:
            f.writelines(lines[:idx])

        functionParams = [os.path.abspath(get_file_global('path_to_srec_cat_exe')),
                          path_in,
                          "-ASCIIHEX",
                          "-exclude",
                          hex(enc_start_addr),
                          hex(enc_end_addr),
                          _pathToTemp,
                          "-ASCIIHEX",
                          "-o",
                          path_out,
                          "-ASCIIHEX"]
        process = subprocess.Popen(functionParams, stdout=subprocess.PIPE, encoding='utf_8')
        for line in iter(process.stdout.readline, ''):
            print(f"encrypt>>>>>{line}")

        # Write IV to corresponding position in image
        ASCIIHEX_WriteToFile_Addr(img_path=path_in,
                                  wrt_addr=get_addr_from_fixedmemloc_h(location_name="APP_AES256CTR_IV_LOC"),
                                  wrt_bytes=self.iv,
                                  wrt_len=self.iv_length
                                  )

    def calculate_image_hash(self, path_to_image, start_addr, end_addr, write_to_addr=None, print_hash_in_file=None):
        _pathToTemp = os.path.dirname(os.path.abspath(path_to_image)) + '\\hash_temp.txt'

        functionParams = [os.path.abspath(get_file_global('path_to_srec_cat_exe')),
                          path_to_image,
                          "-ASCIIHEX",
                          "-crop",
                          hex(start_addr),
                          hex(end_addr),
                          "-fill",
                          "0xff",
                          hex(start_addr),
                          hex(end_addr),
                          "-o",
                          _pathToTemp,
                          "-ASCIIHEX"]

        process = subprocess.Popen(functionParams, stdout=subprocess.PIPE, encoding='utf_8')
        for line in iter(process.stdout.readline, ''):
            print(f"calc_img_hash>>>>>{line}")

        with open(_pathToTemp, 'r') as fr:
            lines = fr.readlines()
            if len(lines) < 3:
                raise Exception('Image file does not contain FW image.')

        # Hash the data
        chosen_hash = hashes.SHA256()
        hasher = hashes.Hash(chosen_hash, default_backend())

        # Calculate Hash line[1:], skip the 1st lines
        for idx in range(1, len(lines), 1):
            # Convert line format to bytes
            data = ASCIIHEX_LinetoBytes(lines[idx])
            if data != b'':
                hasher.update(data)
            else:
                break

        digest = hasher.finalize()

        if write_to_addr is not None:
            ASCIIHEX_WriteToFile_Addr(path_to_image, write_to_addr, digest, len(digest))

        if print_hash_in_file is not None:
            with open(print_hash_in_file, 'wb+') as fw_hash:
                fw_hash.write(digest)
        return digest
