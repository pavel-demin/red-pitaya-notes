---
layout: page
title: Multiband WSPR receiver
permalink: /sdr-receiver-wspr/
---

This is a work in progress...

Interesting links
-----

Some interesting links on the Weak Signal Propagation Reporter (WSPR) protocol:

 - [WSPR home page](http://physics.princeton.edu/pulsar/k1jt/wspr.html)
 - [WSPR 3.0 User's Guide](http://physics.princeton.edu/pulsar/k1jt/WSPR_3.0_User.pdf)
 - [WSPRnet](http://wsprnet.org)
 - [WSPRnet map](http://wsprnet.org/drupal/wsprnet/map)
 - [WSPRnet protocol](http://wsprnet.org/automate.txt)

Short description
-----

The idea of this project is to build a standalone multiband WSPR receiver with all the WSPR decoding done by Red Pitaya.

Hardware
-----

The FPGA configuration consists of eight identical digital down-converters (DDC). Their structure is shown on the following diagram:

![WSPR receiver]({{ "/img/sdr-receiver-wspr.png" | prepend: site.baseurl }})

The DDC output contains complex 32-bit floating-point data at 375 samples per second and is directly compatible with the [WSPR decoder](https://sourceforge.net/p/wsjt/wsjt/HEAD/tree/branches/wsjtx/lib/wsprd/).

The [projects/sdr_receiver_wspr](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver_wspr) directory contains two Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_receiver_wspr/block_design.tcl) and [rx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_receiver_wspr/rx.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

Software
-----

The [write-c2-files.c](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver_wspr/write-c2-files.c) program accumulates 42000 samples at 375 samples per second for each of the eight bands and saves the samples to eight .c2 files.

The recorded .c2 files are processed with the [WSPR decoder](https://sourceforge.net/p/wsjt/wsjt/HEAD/tree/branches/wsjtx/lib/wsprd/).

The decoded data are uploaded to [wsprnet.org](http://wsprnet.org) using [curl](https://curl.haxx.se).

Getting started
-----

A pre-built SD card image can be downloaded from [this link](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/SDR/red-pitaya-wspr-debian-8.2-armhf-20160318.zip).

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
source /opt/Xilinx/Vivado/2015.4/settings64.sh
source /opt/Xilinx/SDK/2015.4/settings64.sh
{% endhighlight %}

Cloning the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
{% endhighlight %}

Building `boot.bin`, `devicetree.dtb` and `uImage`:
{% highlight bash %}
make NAME=sdr_receiver_wspr all
{% endhighlight %}

Building a bootable SD card image:
{% highlight bash %}
sudo sh scripts/image.sh scripts/debian-wspr.sh red-pitaya-wspr-debian-8.2-armhf.img 1024
{% endhighlight %}
