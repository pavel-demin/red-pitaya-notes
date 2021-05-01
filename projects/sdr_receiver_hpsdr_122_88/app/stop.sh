#! /bin/sh

killall -q sdr-receiver-hpsdr

ip link del mvl0

echo 0 > /proc/sys/net/ipv4/conf/all/arp_announce
echo 0 > /proc/sys/net/ipv4/conf/all/arp_ignore
echo 1 > /proc/sys/net/ipv4/conf/all/rp_filter
