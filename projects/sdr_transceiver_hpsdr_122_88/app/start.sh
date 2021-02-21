#! /bin/sh

apps_dir=/media/mmcblk0p1/apps

source $apps_dir/stop.sh

cat $apps_dir/sdr_transceiver_hpsdr_122_88/sdr_transceiver_hpsdr_122_88.bit > /dev/xdevcfg

$apps_dir/sdr_transceiver_hpsdr_122_88/sdr-transceiver-hpsdr 1 2 2 2 1 2 &
