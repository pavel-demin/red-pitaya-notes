#! /bin/sh

apps_dir=/media/mmcblk0p1/apps

source $apps_dir/stop.sh

cat $apps_dir/sdr_transceiver_wide/sdr_transceiver_wide.bit > /dev/xdevcfg

$apps_dir/sdr_transceiver_wide/sdr-transceiver-wide &
