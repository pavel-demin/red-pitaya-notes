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

The [write-c2-files.c](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver_wspr/write-c2-files.c) program accumulates 45000 samples at 375 samples per second for each of the eight bands
and saves the samples to .c2 files

The recorded .c2 files are processed with the [WSPR decoder](https://sourceforge.net/p/wsjt/wsjt/HEAD/tree/branches/wsjtx/lib/wsprd/).

The decoded data are uploaded to [wsprnet.org](http://wsprnet.org) using [curl](https://curl.haxx.se).

Getting started
-----

 - Download customized [SD card image zip file](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/SDR/ecosystem-0.92-65-35575ed-sdr-receiver-wspr.zip).
 - Copy the content of the SD card image zip file to an SD card.
 - Insert the SD card in Red Pitaya and connect the power.

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

Building `sdr_receiver_wspr.bit`:
{% highlight bash %}
make NAME=sdr_receiver_wspr tmp/sdr_receiver_wspr.bit
{% endhighlight %}

Building `write-c2-files`:
{% highlight bash %}
arm-xilinx-linux-gnueabi-gcc projects/sdr_receiver_wspr/server/write-c2-files.c -o write-c2-files -lm -static
{% endhighlight %}

Building `boot.bin`, `devicetree.dtb` and `uImage`:
{% highlight bash %}
make NAME=red_pitaya_0_92 all
{% endhighlight %}

Building SD card image zip file:
{% highlight bash %}
source scripts/sdr-receiver-wspr-ecosystem.sh
{% endhighlight %}
