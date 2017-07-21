#! /bin/sh

DIR=`readlink -f $0`
DIR=`dirname $DIR`

TRANSMITTER=$DIR/transmit-wspr-message
CONFIG=transmit-wspr-message.cfg

GPIO=$DIR/gpio-output

date

test $DIR/$CONFIG -ot $CONFIG || cp $DIR/$CONFIG $CONFIG

echo "Sleeping ..."

SECONDS=`date '+\`expr 59 - %-S\`.\`expr 999999999 - %-N\`'`
sleep `eval echo $SECONDS`

$GPIO 1
sleep 1

date
TIMESTAMP=`date --utc +'%y%m%d_%H%M'`

echo "Transmitting ..."

killall -v $TRANSMITTER
$TRANSMITTER $CONFIG
