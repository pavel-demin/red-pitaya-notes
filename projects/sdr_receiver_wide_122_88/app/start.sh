#! /bin/sh

apps_dir=/media/mmcblk0p1/apps

source $apps_dir/stop.sh

cat $apps_dir/sdr_receiver_wide_122_88/sdr_receiver_wide_122_88.bit > /dev/xdevcfg

$apps_dir/sdr_receiver_wide_122_88/sdr-receiver-wide &
