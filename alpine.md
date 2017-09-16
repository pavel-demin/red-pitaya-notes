---
layout: page
title: Alpine with pre-built applications
permalink: /alpine/
---

This is a work in progress...

Introduction
-----

To simplify maintenance and distribution of the pre-built applications described in the Red Pitaya notes, I've put together a bootable SD card image based on the lightweight [Alpine Linux](https://alpinelinux.org) distribution.

Getting started
-----

 - Download [SD card image zip file](https://www.dropbox.com/sh/5fy49wae6xwxa8a/AADYU-1WH496VRW0cEMIFWlza/red-pitaya-alpine-3.6-armhf-20170916.zip?dl=1).
 - Copy the content of the SD card image zip file to an SD card.
 - Optionally, to start one of the applications automatically at boot time copy its `start.sh` file from `apps/<application>` to the topmost directory on the SD card.
 - Insert the SD card in Red Pitaya and connect the power.
 - Applications can be started from the web interface.

The default password for the `root` account is `changeme`.

Wi-Fi is by default configured in hotspot mode with the network name (SSID) and password both set to `RedPitaya`.

Useful commands
-----

The [Alpine Wiki](http://wiki.alpinelinux.org) contains a lot of information about administrating [Alpine Linux](https://alpinelinux.org). The following is a list of some useful commands.

Switching to client Wi-Fi mode:
{% highlight bash %}
# configure WPA supplicant
wpa_passphrase SSID PASSPHRASE > /etc/wpa_supplicant/wpa_supplicant.conf

# configure services for client Wi-Fi mode
./wifi/client.sh

# save configuration changes to SD card
lbu commit -d
{% endhighlight %}

Switching to hotspot Wi-Fi mode:
{% highlight bash %}
# configure services for hotspot Wi-Fi mode
./wifi/hotspot.sh

# save configuration changes to SD card
lbu commit -d
{% endhighlight %}

Changing password:
{% highlight bash %}
passwd

lbu commit -d
{% endhighlight %}

Installing packages:
{% highlight bash %}
apk add python3

lbu commit -d
{% endhighlight %}

Editing WSPR configuration:
{% highlight bash %}
# make SD card writable
rw

# edit decode-wspr.sh
nano apps/sdr_transceiver_wspr/decode-wspr.sh

# make SD card read-only
ro
{% endhighlight %}
