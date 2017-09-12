#! /bin/sh

apps_dir=/media/mmcblk0p1/apps

source $apps_dir/stop.sh

cat $apps_dir/sdr_transceiver/sdr_transceiver.bit > /dev/xdevcfg

$apps_dir/sdr_transceiver/sdr-transceiver 1 &
$apps_dir/sdr_transceiver/sdr-transceiver 2 &
