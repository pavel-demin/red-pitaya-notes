#! /bin/sh

# CALL and GRID should be specified to enable uploads
CALL=
GRID=

test -n "$CALL" -a -n "$GRID" || exit

SLEEP=$DIR/sleep-rand-100

date

echo "Sleeping ..."

$SLEEP

date
TIMESTAMP=`date --utc +'%y%m%d_%H%M%S'`

echo "Uploading ..."

REPORT=report_$TIMESTAMP.txt

for file in `find . -name decodes_\*.txt -mmin +1`
do
  cat $file >> $REPORT
  rm -f $file
done

# sort by highest SNR, then print unique band/call combinations,
# and then sort them by date/time/frequency

sort -nr -k 4,4 $REPORT | awk '!seen[int($6/1e6)"_"$7]{print} {++seen[int($6/1e6)"_"$7]}' | sort -n -k 1,1 -k 2,2 -k 6,6 -o $REPORT

rm -f $REPORT
