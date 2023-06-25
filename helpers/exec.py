import os
import sys

file_name = os.path.splitext(sys.argv[0])[0] + ".py"
exec(open(file_name).read())
