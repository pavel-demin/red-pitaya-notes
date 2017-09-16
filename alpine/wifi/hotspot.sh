#! /bin/sh

openrc default

rc-update del wpa_supplicant wifi

rc-update add iptables wifi
rc-update add dnsmasq wifi
rc-update add hostapd wifi

sed -i '/^#interface wlan0/{s/^#interface/interface/;n;s/^#static/static/}' /etc/dhcpcd.conf

service dhcpcd restart
