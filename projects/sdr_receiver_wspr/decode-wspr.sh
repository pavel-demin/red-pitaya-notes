#! /bin/sh

# This script is based on the WSPR decoder script by DJ0ABR
# http://www.dj0abr.de/german/technik/dds/wsprbanana_console.htm
# http://www.dj0abr.de/english/technik/dds/wsprbanana_console.htm

DECODER=/root/wsprd_exp
ALLMEPT=ALL_WSPR.TXT

echo "Decoding ..."
for file in *.c2
do
  $DECODER -w $file
  rm -f $file
done

# check if spots are available
FILESIZE=`stat -c%s $ALLMEPT`
echo "Data size: $FILESIZE"
if [ $FILESIZE -ne 0 ]
then
  echo "Uploading ..."

  # ping helps curl to contact the DNS server under various conditions
  ping -W 2 -c 1 wsprnet.org > /dev/null

  # to enable uploading of spots to wsprnet.org specify your call and locator
  # curl -m 8 -F allmept=@$ALLMEPT -F call=MYCALL -F grid=MYLOCATOR http://wsprnet.org/post > /dev/null

  RESULT=$?

  # check if curl uploaded the data successfully
  if [ $RESULT -eq 0 ]
  then
    echo "Upload succeed, deleting $ALLMEPT ..."
    rm -f $ALLMEPT
  else
    echo "Upload failed, error code: $RESULT"
  fi
fi
