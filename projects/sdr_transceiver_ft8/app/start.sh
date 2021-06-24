#! /bin/sh

apps_dir=/media/mmcblk0p1/apps

source $apps_dir/stop.sh

if grep -q '' $apps_dir/sdr_transceiver_ft8/upload-ft8.sh
then
  mount -o rw,remount /media/mmcblk0p1
  dos2unix $apps_dir/sdr_transceiver_ft8/upload-ft8.sh
  mount -o ro,remount /media/mmcblk0p1
fi

rm -rf /dev/shm/*

cat $apps_dir/sdr_transceiver_ft8/sdr_transceiver_ft8.bit > /dev/xdevcfg

ln -sf $apps_dir/sdr_transceiver_ft8/ft8.cron /etc/cron.d/ft8

service dcron restart
