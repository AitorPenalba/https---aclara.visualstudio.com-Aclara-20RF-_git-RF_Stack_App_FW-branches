import os
import json


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


if __name__ == "__main__":
    init_file_globals(path="C:\\temp\\file_globals.json")

    set_file_global(name="doug", value="1")

    get_file_global(name="doug")
