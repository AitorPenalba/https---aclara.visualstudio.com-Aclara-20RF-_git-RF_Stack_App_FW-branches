import sys
import os
import subprocess
import shutil
from utility import get_file_global


def run_pc_lint(path_to_output_dir):
    # setting up dir paths and files to capture PClint output, then create error log
    output_dir = os.path.join(path_to_output_dir)
    pclint_dir = os.path.join(os.path.abspath(get_file_global(name='project_path')), "PCLint")
    pclint_bat = os.path.join(os.path.abspath(pclint_dir), 'lintplus.bat')

    project_name = get_file_global(name='iar_app_target_name')

    pclint_exe_path = os.path.abspath(get_file_global(name='path_to_pclint'))

    shutil.copy(pclint_exe_path, os.path.join(pclint_dir, 'pclp64.exe'))

    os.chdir(pclint_dir)

    with open("pclintlog.txt", 'w') as fw:
        functionParams = [pclint_bat, project_name]
        process = subprocess.Popen(functionParams, stdout=fw, stderr=fw, encoding='utf_8')
        while process.poll() is None:
            pass

    shutil.move(os.path.join(pclint_dir, 'pclintlog.txt'), os.path.join(path_to_output_dir, 'pclintlog.txt'))
    shutil.move(os.path.join(pclint_dir, 'lintoutput.txt'), os.path.join(path_to_output_dir, 'lintoutput.txt'))


    # Setting up counters and everything needed to parse error lintoutput log

    results_file = open(os.path.join(path_to_output_dir, 'lintoutput.txt'), 'r')
    lines = results_file.readlines()
    results_file.close()

    info_count = 0
    warning_count = 0
    error_count = 0
    supplemental_count = 0
    other_count = 0

    num_lines = len(lines)
    current_line_index = 0

    # going through the actual log looking for errors, info, warning etc.
    while current_line_index < num_lines:
        line = lines[current_line_index]

        if line[0:4] == "--- ":
            current_line_index += 1
        elif line.strip() == "":
            current_line_index += 1
        elif line.strip() == "^":
            current_line_index += 1
        elif "  info " in line:
            print(
                f'##[error] PCLintInfo: {line.strip()}{lines[current_line_index + 1]}{lines[current_line_index + 2]}')
            info_count += 1
            current_line_index += 3
        elif "  warning " in line:
            print(
                f'##[error] PCLintWarning: {line.strip()}{lines[current_line_index + 1]}{lines[current_line_index + 2]}')
            warning_count += 1
            current_line_index += 3
        elif "  error " in line:
            print(
                f'##[error] PCLintError: {line.strip()}{lines[current_line_index + 1]}{lines[current_line_index + 2]}')
            error_count += 1
            current_line_index += 3
        elif "  supplemental " in line:
            supplemental_count += 1
            current_line_index += 1
        else:
            other_count += 1
            current_line_index += 1

    print(f"There were {error_count} errors, {warning_count} warnings, {info_count} info messages, {supplemental_count} supplemental messages, and {other_count} other lines that need attention. Please review the lint output file for details.\n")

    if (info_count + warning_count + error_count + supplemental_count + other_count) > 0:
        sys.stderr.write(f"There were {error_count} errors, {warning_count} warnings, {info_count} info messages, {supplemental_count} supplemental messages, and {other_count} other lines that need attention\n")
        print('##vso[task.complete result=Failed;]')
    else:
        print('##vso[task.complete result=Succeeded;]')
