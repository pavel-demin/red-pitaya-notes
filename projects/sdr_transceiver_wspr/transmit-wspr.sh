#! /bin/sh

DIR=`readlink -f $0`
DIR=`dirname $DIR`

TRANSMITTER=$DIR/transmit-wspr-message
CONFIG=$DIR/transmit-wspr-message.cfg

GPIO=$DIR/gpio-output

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
