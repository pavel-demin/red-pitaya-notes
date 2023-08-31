#! /bin/sh

apps_dir=/media/mmcblk0p1/apps

source $apps_dir/stop.sh

cat $apps_dir/radar_122_88/radar_122_88.bit > /dev/xdevcfg

$apps_dir/radar_122_88/radar &
