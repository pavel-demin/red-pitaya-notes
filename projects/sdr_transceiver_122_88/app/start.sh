#! /bin/sh

apps_dir=/media/mmcblk0p1/apps

source $apps_dir/stop.sh

cat $apps_dir/sdr_transceiver_122_88/sdr_transceiver_122_88.bit > /dev/xdevcfg

$apps_dir/sdr_transceiver_122_88/sdr-transceiver 1 &
$apps_dir/sdr_transceiver_122_88/sdr-transceiver 2 &
