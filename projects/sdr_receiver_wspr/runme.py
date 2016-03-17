#!/usr/bin/env python2
###################################################################################################
#
# Copyright 2016 - Clemens Heese / PA7T
#
# This file is part of runme.py.
#
# runmy.py is free software: you can redistribute it and/or modify it under the terms of the GNU
# General Public License as published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# runme.py is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with Foobar. If not, see
# http://www.gnu.org/licenses/.
#
###################################################################################################
#
# Add following line to /etc/fstab to create a ramdisk for the temporary wspr data, otherwise over
# time the sd card will be dead.
#
# tmpfs    /wsprtmp    tmpfs    defaults,size=100M      0       0
#
# To mount it first, mkdir /wsprtmp, and than, mount /wsprtmp. Next reboot mounts it automatically.
#
# Create a user called wspr with 'adduser wspr; adduser wspr kmem' and place runme.py and
# decode_wspr.sh in its home folder /home/wspr. Make sure wspr has access to /dev/mem.
#
# To enable uploading of spots to wsprnet.org update your call and loc in decode_wspr.sh ... at:
# curl -m 8 -F allmept=@wsprdsum.out -F call=MYCALL -F grid=MYLOCATOR ... 
#
# To start the wspr receiver / spotter enter a 'screen' and execute runme.py in an inf-loop:
#
# while true; do sleep 1; ./runme.py; done   
#               
###################################################################################################

import sys
import os
import time
from datetime import datetime
from time import gmtime, strftime

print('Initialized. waiting for even minute...')
t = datetime.utcnow()
sleeptime = 60 - (t.second + t.microsecond/1000000.0)
if (t.minute % 2==0):
    sleeptime += 60
time.sleep(sleeptime)

print('Start recording...')
os.system('cd /wsprtmp; /home/wspr/write-c2-files')
print('Stopped recording.')

# start decoding in background
os.system('/home/wspr/decode_wspr.sh &')


