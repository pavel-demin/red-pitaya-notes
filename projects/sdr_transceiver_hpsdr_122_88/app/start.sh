#! /bin/sh

apps_dir=/media/mmcblk0p1/apps

source $apps_dir/stop.sh

cat $apps_dir/sdr_transceiver_hpsdr_122_88/sdr_transceiver_hpsdr_122_88.bit > /dev/xdevcfg

$apps_dir/sdr_transceiver_hpsdr_122_88/sdr-transceiver-hpsdr 1 2 2 2 1 2 &

address=`awk -F : '$5="FF"' OFS=: /sys/class/net/eth0/address`

echo 2 > /proc/sys/net/ipv4/conf/all/arp_announce
echo 1 > /proc/sys/net/ipv4/conf/all/arp_ignore
echo 2 > /proc/sys/net/ipv4/conf/all/rp_filter

ip link add mvl0 link eth0 address $address type macvlan mode passthru

$apps_dir/sdr_receiver_122_88/sdr-receiver mvl0 &
