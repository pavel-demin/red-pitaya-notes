import os
import sys
import subprocess as sp

if len(sys.argv) != 2:
    sys.exit(1)

uic = os.path.join("Lib", "site-packages", "PySide6", "uic.exe")
cmd = [uic, "-g", "python", sys.argv[1]]
sys.exit(sp.run(cmd, stderr=sp.PIPE, shell=True).returncode)
