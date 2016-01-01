---
layout: page
title: Embedded SDR transceiver
permalink: /sdr-transceiver-emb/
---

Introduction
-----

This version of the Red Pitaya SDR transceiver may be useful for building a small standalone SDR transceiver with all the SDR algorithms running on the on-board CPU of the Red Pitaya board.

Hardware
-----

The structure of this version is very similar to the SDR transceiver described at [this link]({{ "/sdr-transceiver/" | prepend: site.baseurl }}). The two main differences are:

 - FIR filters for fractional sample rate conversion,
 - lower sample rates (24, 48, 96 kSPS).

The basic blocks of the digital down-converters (DDC) and of the digital up-converters (DUC) are shown on the following diagram:

![Embedded SDR transceiver]({{ "/img/sdr-transceiver-emb.png" | prepend: site.baseurl }})

The [projects/sdr_transceiver_emb](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_emb) directory contains four Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_emb/block_design.tcl), [trx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_emb/trx.tcl), [rx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_emb/rx.tcl), [tx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_emb/tx.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

Software
-----

The [projects/sdr_transceiver_emb/gnuradio](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_emb/gnuradio) directory contains [GNU Radio](http://gnuradio.org) blocks, a few examples written in Python and a few flow graph configurations for [GNU Radio Companion](http://gnuradio.org/redmine/projects/gnuradio/wiki/GNURadioCompanion).

Getting started
-----

A pre-built SD card image can be downloaded from [this link](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/SDR/red-pitaya-gnuradio-debian-8.2-armhf-20160101.zip).

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

Running GNU Radio on Red Pitaya
-----

 - Connect a USB sound card to the USB port on the Red Pitaya board.
 - Run SSB transceiver example:
{% highlight bash %}
cd gnuradio
export GRC_BLOCKS_PATH=.
grcc -d . -e trx_ssb.grc
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
make NAME=sdr_transceiver_emb all
{% endhighlight %}

Building a bootable SD card image:
{% highlight bash %}
sudo sh scripts/image.sh scripts/debian-gnuradio.sh red-pitaya-gnuradio-debian-8.2-armhf.img 1024
{% endhighlight %}
