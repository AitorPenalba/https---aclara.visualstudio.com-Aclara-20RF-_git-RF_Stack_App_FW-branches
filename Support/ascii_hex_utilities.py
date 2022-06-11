import os
import subprocess
from utility import get_file_global


def ASCIIHEX_LinetoBytes(line):
    """
    Convert a line(str) in ASCIIHEX file to bytes
    Start fomr the first two character until the end of meet the none Hex character
    :param str line:
    :return: bytes
    """
    line = line.replace(' ', '')
    _idx = 0
    ret = b''
    try:
        while _idx < len(line):
            ret += bytes(bytearray.fromhex(line[_idx:_idx+2]))
            _idx += 2
    except Exception:
        pass
    return ret  # bytes(bytearray.fromhex(line[:-1]))  # Discard'\n' and concert to bytes


def ASCIIHEX_WriteToFile(img_path, offset, wrt_bytes, wrt_len):
    """
    Write bytes to ASCIIHEX format file
    :param file img_path:
    :param int offset: offset from the beginning of the image address
    :param bytes wrt_bytes:
    :param int wrt_len:
    :return: None
    """
    with open(img_path, 'r') as fr:
        lines = fr.readlines()
        if len(lines) < 3:
            raise Exception('Image file does not contain FW image.')

    # Write Value to the Image
    _offset = 0
    _cnt = 0
    for idx in range(1, len(lines), 1):
        # Convert line format to bytes
        data = ASCIIHEX_LinetoBytes(lines[idx])

        _inline_idx = 0
        while (_offset < offset) and (_inline_idx < len(data)):
            _inline_idx = _inline_idx + 1
            _offset = _offset + 1

        if (_offset >= offset) and (_inline_idx < len(data)):
            _buff = wrt_bytes[_cnt:_cnt + len(data) - _inline_idx]
            data = data[:_inline_idx] + _buff + data[_inline_idx + len(_buff):]
            _cnt = _cnt + len(_buff)

            # Bytes convert to Line format
            data = ASCIIHEX_BytestoLine(data)
            # Write date to line
            lines[idx] = data

        if _cnt is wrt_len:
            break

    with open(img_path, 'w') as fw:
        fw.writelines(lines)


def ASCIIHEX_BytestoLine(byte_data):
    """
    Convert bytes to ASCIIHEX format(str)
    :param bytes byte_data:
    :return: str
    """
    byte_data = byte_data.hex().upper()
    return ' '.join([byte_data[i:i+2] for i in range(0, len(byte_data), 2)]) + '\n'


def ASCIIHEX_WriteToFile_Addr(img_path, wrt_addr, wrt_bytes, wrt_len):
    _pathToTemp = os.path.dirname(os.path.abspath(img_path)) + '\\wrt_temp.txt'

    functionParams = [os.path.abspath(get_file_global('path_to_srec_cat_exe')),
                      img_path,
                      "-ASCIIHEX",
                      "-crop",
                      hex(wrt_addr),
                      hex(wrt_addr+wrt_len),
                      "-fill",
                      "0xff",
                      hex(wrt_addr),
                      hex(wrt_addr+wrt_len),
                      "-offset",
                      '-'+hex(wrt_addr),
                      "-o",
                      _pathToTemp,
                      "-ASCIIHEX"]

    process = subprocess.Popen(functionParams, stdout=subprocess.PIPE, encoding='utf_8')
    for line in iter(process.stdout.readline, ''):
        print(f"ascii>>>>>{line}")

    ASCIIHEX_WriteToFile(_pathToTemp, 0, wrt_bytes, wrt_len)

    functionParams = [os.path.abspath(get_file_global('path_to_srec_cat_exe')),
                      img_path,
                      "-ASCIIHEX",
                      "-exclude",
                      hex(wrt_addr),
                      hex(wrt_addr+wrt_len),
                      _pathToTemp,
                      "-ASCIIHEX",
                      "-IGnore-Checksums",
                      "-offset",
                      hex(wrt_addr),
                      "-crop",
                      hex(wrt_addr),
                      hex(wrt_addr + wrt_len),
                      "-o",
                      img_path,
                      "-ASCIIHEX"]
    process = subprocess.Popen(functionParams, stdout=subprocess.PIPE, encoding='utf_8')
    for line in iter(process.stdout.readline, ''):
        print(f"ascii>>>>>{line}")


def ASCIIHEX_ConvertToBinary(img_path, str_addr, end_addr, bin_path):
    """
    This function convert the ASCIIHEX format image file into raw binary data
    :param file img_path: ASCIIHEX image file
    :param int str_addr: Conversion staring address
    :param int end_addr: Conversion end address. If you what the file contain the data from 0x0001 to 0x01FF,
                         then the end_address should be 0x00200
    :param file bin_path: Output  binary file
    :return:
    """
    _pathToTemp = bin_path
    _cal_len = end_addr - str_addr

    functionParams = [os.path.abspath(get_file_global('path_to_srec_cat_exe')),
                      img_path,
                      "-ASCIIHEX",
                      "-crop",
                      hex(str_addr),
                      hex(end_addr),
                      "-offset",
                      '-' + hex(str_addr),
                      "-fill",
                      "0xff",
                      hex(0),
                      hex(_cal_len + 16 - _cal_len % 16),  # _ASCIIHEX_Line_bytes = 16
                      "-o",
                      _pathToTemp,
                      "-Ascii_Hex"]
    process = subprocess.Popen(functionParams, stdout=subprocess.PIPE, encoding='utf_8')
    for line in iter(process.stdout.readline, ''):
        print(f"ascii>>>>>{line}")

    img_binary = b''
    with open(_pathToTemp, 'r') as fr:
        lines = fr.readlines()
        if len(lines) < 2:
            raise Exception('File does not contain data.')
        else:
            _cntData = 0
            for idx in range(1, len(lines) - 2, 1):
                # Convert line format to bytes
                data = ASCIIHEX_LinetoBytes(lines[idx])

                _inline_idx = 0
                while (_cntData < _cal_len) and (_inline_idx < len(data)):
                    _inline_idx = _inline_idx + 1
                    _cntData = _cntData + 1

                # Convert to Binary
                if (_cntData <= _cal_len) and (_inline_idx <= len(data)):
                    img_binary = img_binary + data[:_inline_idx]

    with open(_pathToTemp, 'wb+') as fw:
        fw.write(img_binary)


def ASCIIHEX_OffsetImgToAddr(img_path, start_addr, end_addr, offset_to_addr):
    functionParams = [os.path.abspath(get_file_global('path_to_srec_cat_exe')),
                      img_path,
                      "-ASCIIHEX",
                      "-crop",
                      hex(start_addr),
                      hex(end_addr),
                      "-offset", '-' + hex(start_addr),
                      "-offset",
                      hex(offset_to_addr),
                      "-o",
                      img_path,
                      "-Ascii_Hex"]
    process = subprocess.Popen(functionParams, stdout=subprocess.PIPE, encoding='utf_8')
    for line in iter(process.stdout.readline, ''):
        print(f"ascii>>>>>{line}")
