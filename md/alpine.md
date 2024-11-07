---
title: Alpine with pre-built applications
---

## Introduction

To simplify maintenance and distribution of the pre-built applications described in the Red Pitaya notes, I've put together a bootable SD card image based on the lightweight [Alpine Linux](https://alpinelinux.org) distribution.

## Getting started

- Download [SD card image zip file]({{ site.release_image }}).
- Copy the contents of the SD card image zip file to a micro SD card.
- Optionally, to start one of the applications automatically at boot time, copy its `start.sh` file from `apps/<application>` to the topmost directory on the SD card.
- Install the micro SD card in the Red Pitaya board and connect the power.
- Applications can be started from the web interface.

The default password for the `root` account is `changeme`.

## Network configuration

Wi-Fi is by default configured in hotspot mode with the network name (SSID) and password both set to `RedPitaya`. When in hotspot mode, the IP address of Red Pitaya is [192.168.42.1](http://192.168.42.1).

The wired interface is by default configured to request an IP address via DHCP. If no IP address is provided by a DHCP server, then the wired interface falls back to a static IP address [192.168.1.100](http://192.168.1.100).

The configuration of the IP addresses is in [/etc/dhcpcd.conf](https://github.com/pavel-demin/red-pitaya-notes/blob/master/alpine/etc/dhcpcd.conf). More information about [/etc/dhcpcd.conf](https://github.com/pavel-demin/red-pitaya-notes/blob/master/alpine/etc/dhcpcd.conf) can be found at [this link](https://www.mankier.com/5/dhcpcd.conf).

From systems with enabled DNS Service Discovery (DNS-SD), Red Pitaya can be accessed as `rp-f0xxxx.local`, where `f0xxxx` are the last 6 characters from the MAC address written on the Ethernet connector.

In the local networks with enabled local DNS, Red Pitaya can also be accessed as `rp-f0xxxx`.

## Useful commands

The [Alpine Wiki](https://wiki.alpinelinux.org) contains a lot of information about administrating [Alpine Linux](https://alpinelinux.org). The following is a list of some useful commands.

Switching to client Wi-Fi mode:

```bash
# configure WPA supplicant
wpa_passphrase SSID PASSPHRASE > /etc/wpa_supplicant/wpa_supplicant.conf

# configure services for client Wi-Fi mode
./wifi/client.sh

# save configuration changes to SD card
lbu commit -d
```

Switching to hotspot Wi-Fi mode:

```bash
# configure services for hotspot Wi-Fi mode
./wifi/hotspot.sh

# save configuration changes to SD card
lbu commit -d
```

Changing password:

```bash
passwd

lbu commit -d
```

Installing packages:

```bash
apk add gcc make

lbu commit -d
```

Editing WSPR configuration:

```bash
# make SD card writable
rw

# edit decode-wspr.sh
nano apps/sdr_transceiver_wspr/decode-wspr.sh

# make SD card read-only
ro
```

## Troubleshooting

It is normal that there are no blinking LEDs after booting the Red Pitaya board with this SD card image.

The boot process can be checked using the USB/serial console as explained at [this link](https://redpitaya.readthedocs.io/en/latest/developerGuide/software/console/console/console.html).

The getting started instructions are known to work with a freshly unpacked factory formatted (single partition, FAT32 file system) micro SD card.

If the micro SD card was previously partitioned and formatted for other purposes, then the following commands can be used to format it:

```bash
parted -s /dev/mmcblk0 mklabel msdos
parted -s /dev/mmcblk0 mkpart primary fat32 4MiB 100%
mkfs.vfat -v /dev/mmcblk0p1
```

where `/dev/mmcblk0` is the name of the device corresponding to the micro SD card.

It is also possible to write an empty SD card image with a single FAT32 partition instead of using partitioning and formatting commands. For example, a repository with several empty SD card images can be found at [this link](https://github.com/procount/fat32images).

If the Ethernet interface of the Red Pitaya board is directly connected to the Ethernet interface of a computer, then the Ethernet interface of the computer should be configured to have an IP address in the same 192.168.1.x sub-network. For example, 192.168.1.111. Instructions on how to set a static IP address in Windows can be found at [this link](https://kb.netgear.com/27476/How-do-I-set-a-static-IP-address-in-Windows).
