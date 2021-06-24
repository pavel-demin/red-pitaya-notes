#! /bin/sh

if test -e /etc/cron.d/ft8
then
  rm -f /etc/cron.d/ft8
  service dcron restart
  killall -q decode-ft8.sh sleep-to-59 write-c2-files ft8d
  killall -q upload-ft8.sh sleep-rand
  killall -q update-corr.sh measure-corr measure-level
fi
