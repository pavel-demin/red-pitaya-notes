#! /bin/sh

# CALL and GRID should be specified to enable uploads
CALL=
GRID=

# optional antenna description
ANTENNA=""

DIR=`readlink -f $0`
DIR=`dirname $DIR`

UPLOADER=$DIR/upload-to-pskreporter

SLEEP=/media/mmcblk0p1/apps/common_tools/sleep-rand

date

echo "Sleeping ..."

$SLEEP 60

date
TIMESTAMP=`date --utc +'%y%m%d_%H%M'`

echo "Processing ..."

REPORT=report_$TIMESTAMP.txt

for file in `find . -name decodes_\*.txt -mmin +1`
do
  cat $file >> $REPORT
  rm -f $file
done

test -f $REPORT || exit

# sort by highest SNR, then print unique band/call combinations,
# and then sort them by date/time/frequency
sort -nr -k 4,4 $REPORT | awk '!seen[int($6/1e6)"_"$7]{print} {++seen[int($6/1e6)"_"$7]}' | sort -n -k 1,1 -k 2,2 -k 6,6 -o $REPORT

test -n "$CALL" -a -n "$GRID" || exit

echo "Uploading ..."

$UPLOADER $CALL $GRID "$ANTENNA" $REPORT

rm -f $REPORT
