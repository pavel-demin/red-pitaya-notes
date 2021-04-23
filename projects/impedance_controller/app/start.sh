#! /bin/sh

apps_dir=/media/mmcblk0p1/apps

source $apps_dir/stop.sh

cat $apps_dir/impedance_controller/impedance_controller.bit > /dev/xdevcfg

$apps_dir/impedance_controller/impedance-controller &
