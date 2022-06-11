import os
import sys
import shutil
import subprocess
from utility import get_file_global


def build_doxygen_files(path_to_doxygen_run_from_dir, path_to_doxygen_output_dir):
    family_mtu_type = int(get_file_global(name='firmware_family'))

    # Generate Doxygen File
    if family_mtu_type == 0:
        tmp_dest = os.path.join(os.path.abspath(get_file_global(name='project_path')), "support", "GasGeneralDocs")
    elif family_mtu_type == 1:
        tmp_dest = os.path.join(os.path.abspath(get_file_global(name='project_path')), "support", "GasPulseEvcDocs")
    elif family_mtu_type == 2:
        tmp_dest = os.path.join(os.path.abspath(get_file_global(name='project_path')), "support", "EncoderDocs")
    elif family_mtu_type == 3:
        tmp_dest = os.path.join(os.path.abspath(get_file_global(name='project_path')), "support", "AdvAlarmsDocs")

    error_occurred = False
    try:
        # trying to run doxygen file
        print("- Building Doxygen...")
        cnt_err, cnt_warn = run_doxygen(path_to_doxygen_run_from_dir)
    except FileExistsError as err:
        print("- Build Failed!")
        print("- %s" % err)
        sys.stderr.write("Doxygen failed with an Exception: {}\n".format(err))
        print('##vso[task.complete result=Failed;]')
        error_occurred = True
    else:
        print("  Error %d, Warning %d" % (cnt_err, cnt_warn))
        if (cnt_err + cnt_warn) > 0:
            sys.stderr.write("Doxygen failed with {} errors and {} warnings\n".format(cnt_err, cnt_warn))
            print('##vso[task.complete result=Failed;]')
            error_occurred = True

    if os.path.exists(os.path.join(tmp_dest, "html", "todo.html")):
        sys.stderr.write("To-Do File is Not Empty!\n".format(cnt_err, cnt_warn))
        print('##vso[task.complete result=Failed;]')
        error_occurred = True

    shutil.move(os.path.join(tmp_dest, "html"), path_to_doxygen_output_dir)
    shutil.move(os.path.join(tmp_dest, "latex"), path_to_doxygen_output_dir)
    shutil.move(os.path.join(tmp_dest, "..", "doxylog.txt"), path_to_doxygen_output_dir)

    if not error_occurred:
        print('##vso[task.complete result=Succeeded;]')


def run_doxygen(path_to_doxygen_run_from_dir):
    path_to_doxygen_exe = os.path.abspath(get_file_global('path_to_doxygen_exe'))
    path_to_doxygen_conf = os.path.abspath(get_file_global('path_to_doxygen_conf'))

    original_path = os.getcwd()
    os.chdir(path_to_doxygen_run_from_dir)
    with open("doxylog.txt", 'w') as fw:
        functionParams = [path_to_doxygen_exe, path_to_doxygen_conf]

        process = subprocess.Popen(functionParams, stdout=subprocess.PIPE, encoding='utf_8')
        for line in iter(process.stdout.readline, ''):
            fw.write(line)
            print(line.strip())

    with open("doxylog.txt", 'r') as fr:
        _cnt_err = 0
        _cnt_warn = 0
        for line in iter(fr.readline, ''):
            _cnt_err = _cnt_err + line.count('error')
            _cnt_warn = _cnt_warn + line.count('warning')

    os.chdir(original_path)
    return _cnt_err, _cnt_warn


