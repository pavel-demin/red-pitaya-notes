#!/bin/bash
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
#                    This script is heavily based on work from DJ0ABR.
# 
###################################################################################################

cd /wsprtmp
for f in wspr*.c2
	do date >> spots
	echo "Processing $f ..." >> spots
	wsprd_exp -w $f >>spots
	rm $f

	# check if spots are available
        FILESIZE=$(stat -c%s "wspr_spots.txt")
        echo data size= $FILESIZE >> spots
        if [ $FILESIZE -ne 0 ] ; then

                # add the spots to a temporary file used for uploading to wsprnet.org
                echo add to wsprdsum.out >> spots
                cat wspr_spots.txt >> wsprdsum.out

                # upload the spots
                echo upload by curl >> spots
                # ping helps curl to contact the DNS server under various conditions, i.e. if the internet connection was lost
                ping -W 2 -c 1 wsprnet.org > /dev/null;
                #curl -m 8 -F allmept=@wsprdsum.out -F call=PA7T -F grid=JO22FD http://wsprnet.org/post > /dev/null;
                RESULT=$?

                # check if curl uploaded the data successfully
                # delete only if uploaded
                if [ $RESULT -eq 0 ] ; then
                        # data uploaded, delete them
                        echo Upload OK, deleting >> spots
                        rm wsprdsum.out
                fi
                #rm wspr_spots.txt
                echo curl result: $RESULT , done. >> spots
        fi
done
