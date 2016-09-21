#! /bin/sh

TRANSMITTER=/root/transmit-wspr-message
CONFIG=/root/transmit-wspr-message.cfg

GPIO=/root/gpio-output

date

echo "Sleeping ..."

SECONDS=`date +%S`
sleep `expr 60 - $SECONDS`

$GPIO 1
sleep 1

date
TIMESTAMP=`date --utc +'%y%m%d_%H%M'`

echo "Transmitting ..."

killall -v $TRANSMITTER
$TRANSMITTER $CONFIG
