#! /bin/sh

if test -e /etc/periodic/wspr
then
  rm -f /etc/periodic/wspr
  service dcron restart
  killall -q decode-wspr.sh sleep-to-59 write-c2-files wsprd
  killall -q update-corr.sh measure-corr
fi
