---
layout: page
title: Multiband WSPR receiver
permalink: /sdr-receiver-wspr/
---

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

This project implements a standalone multiband WSPR receiver with all the WSPR signal processing done by Red Pitaya in the following way:

 - simultaneously record WPSR signals from eight bands
 - use FPGA for all the conversions needed to produce .c2 files (complex 32-bit floating-point data at 375 samples per second)
 - use on-board CPU to process the .c2 files with the [WSPR decoder](https://sourceforge.net/p/wsjt/wsjt/HEAD/tree/branches/wsjtx/lib/wsprd/)
 - upload decoded data to [wsprnet.org](http://wsprnet.org)

With this configuration, it is enough to connect Red Pitaya to an antenna and to a network. After switching Red Pitaya on, it will automatically start operating as a WSPR receiver.

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

The [decode-wspr.sh](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver_wspr/decode-wspr.sh) script launches `write-c2-files`, `wsprd` and `curl` one after another. This script is run every two minutes by the following crontab entry:
{% highlight bash %}
1-59/2 * * * * cd /dev/shm && /root/decode-wspr.sh >> decode-wspr.log
{% endhighlight %}

Getting started
-----

An antenna should be connected to the IN1 connector on the Red Pitaya board.

A pre-built SD card image can be downloaded from [this link](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/SDR/red-pitaya-wspr-debian-8.2-armhf-20160322.zip).

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

Configuring WSPR receiver
-----

By default, the uploads to [wsprnet.org](http://wsprnet.org) are disabled and all the decoded data are accumulated in `/dev/shm/ALL_WSPR.TXT`.

To enable uploads, the `CALL` and `GRID` variables should be specified in [decode-wspr.sh](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver_wspr/decode-wspr.sh#L4). These variables should be set to the call sign of the receiving station and its 6-character Maidenhead grid locator.

The frequency correction ppm value can be adjusted by editing the `CORR` variable in [decode-wspr.sh](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver_wspr/decode-wspr.sh#L8).

The bands can be configured by editing and recompiling [write-c2-files.c](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver_wspr/write-c2-files.c). The `freq[8]` array contains all the WSPR frequencies. They can be enabled or disabled by uncommenting or by commenting the corresponding lines. The command to compile [write-c2-files.c](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver_wspr/write-c2-files.c) from the Red Pitaya command line is:
{% highlight bash %}
gcc write-c2-files.c -o write-c2-files -lm
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

Feedback and results
-----

I would like to thank PA7T and DK5HH for their interest in this project, for the tests that they have done and for the valuable feedback that they have provided.

The following plots show the number of WSPR spots per hour decoded by the multiband WSPR receiver:

![WSPR spots by PA7T]({{ "/img/wspr-spots-PA7T.png" | prepend: site.baseurl }})
![WSPR spots by DK5HH]({{ "/img/wspr-spots-DK5HH.png" | prepend: site.baseurl }})
