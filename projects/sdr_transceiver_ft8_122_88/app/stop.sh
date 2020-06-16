#! /bin/sh

if test -e /etc/periodic/ft8
then
  rm -f /etc/periodic/ft8
  service dcron restart
  killall -q decode-ft8.sh sleep-to-59 write-c2-files ft8d
  killall -q update-corr.sh measure-corr
fi
