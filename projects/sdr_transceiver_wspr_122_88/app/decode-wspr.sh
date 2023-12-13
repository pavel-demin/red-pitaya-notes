#! /bin/sh

# CALL and GRID should be specified to enable uploads
CALL=
GRID=

# version information
VERSION=RP122-16

JOBS=4
NICE=10

DIR=`readlink -f $0`
DIR=`dirname $DIR`

RECORDER=$DIR/write-c2-files
CONFIG=write-c2-files.cfg

DECODER=/media/mmcblk0p1/apps/wsprd/wsprd
ALLMEPT=ALL_WSPR.TXT

GPIO=/media/mmcblk0p1/apps/common_tools/gpio-output

SLEEP=/media/mmcblk0p1/apps/common_tools/sleep-to-59

date

test $DIR/$CONFIG -ot $CONFIG || cp $DIR/$CONFIG $CONFIG

echo "Sleeping ..."

$SLEEP

$GPIO 0
sleep 1

date
TIMESTAMP=`date --utc +'%y%m%d_%H%M'`

echo "Recording ..."

killall -q $RECORDER
$RECORDER $CONFIG

echo "Decoding ..."

for file in wspr_*_$TIMESTAMP.c2
do
  while [ `pgrep $DECODER | wc -l` -ge $JOBS ]
  do
    sleep 1
  done
  nice -n $NICE $DECODER -JC 2000 $file &
done

wait

rm -f wspr_*_$TIMESTAMP.c2

test -n "$CALL" -a -n "$GRID" -a -s $ALLMEPT || exit

echo "Uploading ..."

# sort by highest SNR, then print unique date/time/band/call combinations,
# and then sort them by date/time/frequency
sort -nr -k 3,3 $ALLMEPT | awk '!seen[$1"_"$2"_"int($5)"_"$7] {print} {++seen[$1"_"$2"_"int($5)"_"$7]}' | sort -n -k 1,1 -k 2,2 -k 5,5 -o $ALLMEPT

curl -sS -m 30 -F allmept=@$ALLMEPT -F call=$CALL -F grid=$GRID -F version=$VERSION http://wsprnet.org/post > /dev/null

rm -f $ALLMEPT
