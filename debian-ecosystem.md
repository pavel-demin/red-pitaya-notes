---
layout: page
title: Debian with Red Pitaya ecosystem
permalink: /debian-ecosystem/
---

Introduction
-----

I've put together a bootable SD card image for the Red Pitaya board containing the following:

 - Linux 4.6.0-xilinx
 - Debian 8.6 (armhf)
 - Development tools (GCC 4.9.2, make)
 - Wi-Fi drivers for MediaTek/Ralink and Realtek chipsets
 - Wi-Fi access point
 - Red Pitaya SDK (rp.h, librp.so)
 - Red Pitaya command-line tools (acquire, calib, generate, monitor)
 - Red Pitaya web server
 - Red Pitaya contributed apps

With this SD card image, it's possible to develop and compile C programs directly on the Red Pitaya board.

For example, here are the commands to compile all the Red Pitaya SDK examples:

{% highlight bash %}
cp -a /opt/redpitaya/examples .
cd examples
make
{% endhighlight %}

Many other libraries, scripting languages and tools can be installed from the Debian repository.

For example, here are the commands to install the XFCE desktop environment and to export it via VNC:

{% highlight bash %}
apt-get install xfonts-base tightvncserver xfce4-panel xfce4-session xfwm4 xfdesktop4 xfce4-terminal thunar gnome-icon-theme
vncserver -geometry 1200x700
{% endhighlight %}

Getting started
-----

A pre-built SD card image can be downloaded from [this link](https://www.dropbox.com/sh/5fy49wae6xwxa8a/AABxosZOAEpf62XPnr_g7KjOa/red-pitaya-ecosystem/red-pitaya-ecosystem-0.95-debian-8.5-armhf-20160621.zip?dl=1).

The SD card image size is 1 GB, so it should fit on any SD card starting from 2 GB.

To write the image to a SD card, the `dd` command-line utility can be used on GNU/Linux and Mac OS X or [Win32 Disk Imager](http://sourceforge.net/projects/win32diskimager/) can be used on MS Windows.

The default password for the `root` account is `changeme`.

To use the full size of a SD card, the SD card partitions should be resized with the following commands:

{% highlight bash %}
# delete second partition
echo -e "d\n2\nw" | fdisk /dev/mmcblk0
# recreate partition
parted -s /dev/mmcblk0 mkpart primary ext4 16MB 100%
# resize partition
resize2fs /dev/mmcblk0p2
{% endhighlight %}

Building from source
-----

The installation of the development machine is described at [this link]({{ "/development-machine/" | prepend: site.baseurl }}).

The structure of the source code and of the development chain is described at [this link]({{ "/led-blinker/" | prepend: site.baseurl }}).

Setting up the Vivado environment:
{% highlight bash %}
source /opt/Xilinx/Vivado/2016.3/settings64.sh
source /opt/Xilinx/SDK/2016.3/settings64.sh
{% endhighlight %}

Cloning the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
{% endhighlight %}

Building `boot.bin`, `devicetree.dtb` and `uImage`:
{% highlight bash %}
make NAME=led_blinker all
{% endhighlight %}

Building a bootable SD card image:
{% highlight bash %}
sudo sh scripts/image.sh scripts/debian-ecosystem.sh red-pitaya-ecosystem-0.95-debian-8.5-armhf.img 1024
{% endhighlight %}
