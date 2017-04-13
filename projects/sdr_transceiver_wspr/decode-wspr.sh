#! /bin/sh

# CALL and GRID should be specified to enable uploads
CALL=
GRID=

#WSPRD OPTIONS; these options are passed directly to wsprd decoder stack the (examples d deeper inspection, q quick inspection, w wide band inspection)
WSPROPTIONS="-wJC 5000"

#options for paralles
JOBS=2
NICE=10

DIR=`readlink -f $0`
DIR=`dirname $DIR`

RECORDER=$DIR/write-c2-files
CONFIG=$DIR/write-c2-files.cfg

DECODER=$DIR/wsprd/wsprd
ALLMEPT=ALL_WSPR.TXT

GPIO=$DIR/gpio-output

date

echo "Sleeping ..."

SECONDS=`date +%S`
sleep `expr 59 - $SECONDS`

$GPIO 0
sleep 1

date
TIMESTAMP=`date --utc +'%y%m%d_%H%M'`

echo "Recording ..."

killall -v $RECORDER
$RECORDER $CONFIG

echo "Decoding ..."

parallel --keep-order --jobs $JOBS --nice $NICE $DECODER $WSPROPTIONS ::: wspr_*_$TIMESTAMP.c2
rm -f wspr_*_$TIMESTAMP.c2

test -n "$CALL" -a -n "$GRID" -a -s $ALLMEPT || exit

echo "Uploading ..."

# sort by highest SNR, then print unique date/time/band/call combinations,
# and then sort them by date/time/frequency
sort -nr -k 4,4 $ALLMEPT | awk '!seen[$1"_"$2"_"int($6)"_"$7] {print} {++seen[$1"_"$2"_"int($6)"_"$7]}' | sort -n -k 1,1 -k 2,2 -k 6,6 -o $ALLMEPT

curl -sS -m 30 -F allmept=@$ALLMEPT -F call=$CALL -F grid=$GRID http://wsprnet.org/post > /dev/null

rm -f $ALLMEPT
