#! /bin/sh

apps_dir=/media/mmcblk0p1/apps

source $apps_dir/stop.sh

cat $apps_dir/sdr_receiver_hpsdr/sdr_receiver_hpsdr.bit > /dev/xdevcfg

$apps_dir/sdr_receiver_hpsdr/sdr-receiver-hpsdr 1 1 1 1 1 1 1 1 &
