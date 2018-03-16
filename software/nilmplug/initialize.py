
import pkg_resources
import shutil
import os
import sys

ESC_SEQ = "\x1b["
COL_RESET = ESC_SEQ+"0m"
COL_RED = ESC_SEQ+"6;30;41m"
COL_GREEN = ESC_SEQ+"6;30;42m"
COL_YELLOW = ESC_SEQ+"6;30;43m"
COL_BLUE = ESC_SEQ+"34;01m"
COL_MAGENTA = ESC_SEQ+"35;01m"
COL_CYAN = ESC_SEQ+"36;01m"



def run():
    sys.stdout.write("adding udev entry for nilm-plug devices ")
    udev_file = pkg_resources.resource_filename(
        "nilmplug", "resources/90-nilmplug.rules")
    try:
        shutil.copy(udev_file, "/etc/udev/rules.d")
    except(PermissionError):
        run_as_root()        
    print(("["+COL_GREEN+"OK"+COL_RESET+"]"))

def run_as_root():
    print(("["+COL_RED+"ERROR"+COL_RESET+"]\n run as [sudo nilm initialize]"))
    exit(1)
