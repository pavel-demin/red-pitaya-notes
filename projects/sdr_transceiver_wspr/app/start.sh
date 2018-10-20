#! /bin/sh

apps_dir=/media/mmcblk0p1/apps

source $apps_dir/stop.sh

rm -rf /dev/shm/*

cat $apps_dir/sdr_transceiver_wspr/sdr_transceiver_wspr.bit > /dev/xdevcfg

ln -sf $apps_dir/sdr_transceiver_wspr/wspr.cron /etc/periodic/wspr

service dcron restart
