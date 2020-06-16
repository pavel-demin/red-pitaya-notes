#! /bin/sh

killall -q sdr-receiver-hpsdr

echo 0 > /proc/sys/net/ipv4/conf/all/arp_filter
echo 1 > /proc/sys/net/ipv4/conf/all/rp_filter
ip link del mvl0
