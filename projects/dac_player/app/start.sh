#! /bin/sh

apps_dir=/media/mmcblk0p1/apps

source $apps_dir/stop.sh

cat $apps_dir/dac_player/dac_player.bit > /dev/xdevcfg
