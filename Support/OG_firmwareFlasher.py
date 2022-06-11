import subprocess
import os
from fileGlobals import get_file_global


def flash_image(path_to_image):
    """
    This function can be used to program an MTU with a manufacturing image.

    It is just a wrapper for the TI Uniflash 4.4.0 command line interface.

    To use this class some setup is required:
    1) Connect the MSP-FET to the MTU.
    2) Start UniFlash 4.4.0, ensure it autodetects the MTU, click Start
    3) In the left hand pane, click "Standalone Command Line"
        a. Deselect the Images checkbox
        b. Make sure other settings are reasonable.
        c. Click "Generate Package"
        d. This will generate a .zip file, save it to a convenient location
    4) Click the "download ccxml" button in UniFlash (this is at the top of the window, toward the center)
        a. Save the .ccxml file to a convenient location
    5) Close UniFlash
    6) Extract the zip file created in step 3 to a convenient location.
    7) You may need to run "one_time_setup.bat"

    """

    response = subprocess.call([os.path.abspath(get_file_global('path_to_uniflash_batch_file')),
                                "-c",
                                os.path.abspath(get_file_global('path_to_uniflash_ccxml')),
                                "-f",
                                path_to_image])

    return response  # 0 means success
