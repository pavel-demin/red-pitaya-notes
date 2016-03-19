#! /bin/sh

# CALL and GRID should be specified to enable uploads
CALL=
GRID=

JOBS=1
NICE=10

DECODER=/root/wsprd/wsprd_exp
ALLMEPT=ALL_WSPR.TXT

date

echo "Decoding ..."

TIMESTAMP=`date --utc --date='-2min' +'%y%m%d_%H%M'`
find . -name wspr_\*_$TIMESTAMP.c2 | parallel --jobs $JOBS --nice $NICE $DECODER -J -w
rm -f wspr_*_$TIMESTAMP.c2

test -z "$CALL" -o -z "$GRID" && exit

FILESIZE=`stat -c%s $ALLMEPT`
echo "Data size: $FILESIZE"

test $FILESIZE -eq 0 && exit

echo "Uploading ..."

curl -m 8 -F allmept=@$ALLMEPT -F call=$CALL -F grid=$GRID http://wsprnet.org/post > /dev/null && rm -f $ALLMEPT
