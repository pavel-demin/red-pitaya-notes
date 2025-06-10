#! /bin/sh

apps_dir=/media/mmcblk0p1/apps

. $apps_dir/stop.sh

cat $apps_dir/led_blinker_122_88/led_blinker_122_88.bit > /dev/xdevcfg
