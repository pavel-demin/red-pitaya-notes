#! /bin/sh

DIR=`readlink -f $0`
DIR=`dirname $DIR`

TRANSMITTER=$DIR/transmit-wspr-message
CONFIG=transmit-wspr-message.cfg

GPIO=/media/mmcblk0p1/apps/common_tools/gpio-output

SLEEP=/media/mmcblk0p1/apps/common_tools/sleep-to-59

date

test $DIR/$CONFIG -ot $CONFIG || cp $DIR/$CONFIG $CONFIG

echo "Sleeping ..."

$SLEEP

sleep 1
$GPIO 1
sleep 1

date
TIMESTAMP=`date --utc +'%y%m%d_%H%M'`

echo "Transmitting ..."

killall -q $TRANSMITTER
$TRANSMITTER $CONFIG
