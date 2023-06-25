import sys
import subprocess as sp

cmd = ["uic", "-g", "python"] + sys.argv[1:]
sys.exit(sp.run(cmd, stderr=sp.PIPE, shell=True).returncode)
