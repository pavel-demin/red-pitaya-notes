#! /bin/sh

apps_dir=/media/mmcblk0p1/apps

source $apps_dir/stop.sh

cat $apps_dir/pulsed_nmr/pulsed_nmr.bit > /dev/xdevcfg

$apps_dir/pulsed_nmr/pulsed-nmr &
