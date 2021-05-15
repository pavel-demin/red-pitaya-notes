import sys
from subprocess import Popen, PIPE
cmd = ['uic', '-g', 'python'] + sys.argv[1:]
proc = Popen(cmd, stderr=PIPE, shell=True)
out, err = proc.communicate()
sys.exit(proc.returncode)
