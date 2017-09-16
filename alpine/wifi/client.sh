#! /bin/sh

openrc default

rc-update del iptables wifi
rc-update del dnsmasq wifi
rc-update del hostapd wifi

rc-update add wpa_supplicant wifi

sed -i '/^interface wlan0/{s/^interface/#interface/;n;s/^static/#static/}' /etc/dhcpcd.conf

service dhcpcd restart
