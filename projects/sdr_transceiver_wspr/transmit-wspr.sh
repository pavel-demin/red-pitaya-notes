#! /bin/sh

TRANSMITTER=/root/transmit-wspr-message
CONFIG=/root/transmit-wspr-message.cfg

date

echo "Sleeping ..."

SECONDS=`date +%S`
sleep `expr 61 - $SECONDS`

date
TIMESTAMP=`date --utc +'%y%m%d_%H%M'`

echo "Transmitting ..."

killall -v $TRANSMITTER
$TRANSMITTER $CONFIG
