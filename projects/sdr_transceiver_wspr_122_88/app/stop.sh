#! /bin/sh

if test -e /etc/cron.d/wspr_122_88
then
  rm -f /etc/cron.d/wspr_122_88
  service dcron restart
  killall -q decode-wspr.sh sleep-to-59 write-c2-files wsprd
  killall -q update-corr.sh measure-corr measure-level
fi
