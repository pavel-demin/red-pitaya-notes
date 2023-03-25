#! /bin/sh

apps_dir=/media/mmcblk0p1/apps

source $apps_dir/stop.sh

cat $apps_dir/sdr_receiver_slave/sdr_receiver_slave.bit > /dev/xdevcfg

$apps_dir/sdr_receiver_slave/sdr-receiver-slave &
