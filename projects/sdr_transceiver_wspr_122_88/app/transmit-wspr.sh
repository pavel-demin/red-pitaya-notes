#! /bin/sh

DIR=`readlink -f $0`
DIR=`dirname $DIR`

TRANSMITTER=$DIR/transmit-wspr-message
CONFIG=transmit-wspr-message.cfg

GPIO=$DIR/gpio-output

SLEEP=$DIR/sleep-to-59

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
