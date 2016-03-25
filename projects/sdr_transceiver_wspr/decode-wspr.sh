#! /bin/sh

# CALL and GRID should be specified to enable uploads
CALL=
GRID=

# frequency correction in ppm
CORR=0

JOBS=1
NICE=10

RECORDER=/root/write-c2-files
DECODER=/root/wsprd/wsprd_exp
ALLMEPT=ALL_WSPR.TXT

date

echo "Sleeping ..."

SECONDS=`date +%S`
sleep `expr 60 - $SECONDS`

date
TIMESTAMP=`date --utc +'%y%m%d_%H%M'`

echo "Recording ..."

killall -v $RECORDER
$RECORDER $CORR

echo "Decoding ..."

parallel --jobs $JOBS --nice $NICE $DECODER -J -w ::: wspr_*_$TIMESTAMP.c2
rm -f wspr_*_$TIMESTAMP.c2

test -n "$CALL" -a -n "$GRID" -a -s $ALLMEPT || exit

echo "Uploading ..."

curl -m 8 -F allmept=@$ALLMEPT -F call=$CALL -F grid=$GRID http://wsprnet.org/post > /dev/null

test $? -ne 0 || rm -f $ALLMEPT
