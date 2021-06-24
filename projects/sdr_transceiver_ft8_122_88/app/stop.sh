#! /bin/sh

if test -e /etc/cron.d/ft8_122_88
then
  rm -f /etc/cron.d/ft8_122_88
  service dcron restart
  killall -q decode-ft8.sh sleep-to-59 write-c2-files ft8d
  killall -q upload-ft8.sh sleep-rand
  killall -q update-corr.sh measure-corr measure-level
fi
