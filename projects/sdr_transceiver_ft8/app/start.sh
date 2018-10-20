#! /bin/sh

apps_dir=/media/mmcblk0p1/apps

source $apps_dir/stop.sh

rm -rf /dev/shm/*

cat $apps_dir/sdr_transceiver_ft8/sdr_transceiver_ft8.bit > /dev/xdevcfg

ln -sf $apps_dir/sdr_transceiver_ft8/ft8.cron /etc/periodic/ft8

service dcron restart
