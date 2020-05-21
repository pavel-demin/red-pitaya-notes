---
layout: page
title: SDR receiver compatible with HPSDR
permalink: /sdr-receiver-hpsdr/
---

Introduction
-----

This version of the Red Pitaya SDR receiver emulates a [Hermes](http://openhpsdr.org/hermes.php) module with eight receivers. It may be useful for projects that require eight receivers compatible with the programs that support the HPSDR/Metis communication protocol.

The HPSDR/Metis communication protocol is described in the following documents:

 - [Metis - How it works](https://github.com/TAPR/OpenHPSDR-SVN/raw/master/Metis/Documentation/Metis- How it works_V1.33.pdf)

 - [HPSDR - USB Data Protocol](https://github.com/TAPR/OpenHPSDR-SVN/raw/master/Documentation/USB_protocol_V1.58.doc)

Hardware
-----

The FPGA configuration consists of eight identical digital down-converters (DDC). Their structure is shown on the following diagram:

![HPSDR receiver]({{ "/img/sdr-receiver-hpsdr.png" | prepend: site.baseurl }})

The main problem in emulating the HPSDR hardware with Red Pitaya is that the Red Pitaya ADC sample rate is 125 MSPS and the HPSDR ADC sample rate is 122.88 MSPS.

To address this problem, this version contains a set of FIR filters for fractional sample rate conversion.

The resulting I/Q data rate is configurable and three settings are available: 48, 96, 192 kSPS.

The tunable frequency range covers from 0 Hz to 61.44 MHz.

The [projects/sdr_receiver_hpsdr](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver_hpsdr) directory contains two Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_receiver_hpsdr/block_design.tcl), [rx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_receiver_hpsdr/rx.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

The [projects/sdr_receiver_hpsdr/filters](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver_hpsdr/filters) directory contains the source code of the [R](http://www.r-project.org) scripts used to calculate the coefficients of the FIR filters.

The [projects/sdr_receiver_hpsdr/server](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver_hpsdr/server) directory contains the source code of the UDP server ([sdr-receiver-hpsdr.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_receiver_hpsdr/server/sdr-receiver-hpsdr.c)) that receives control commands and transmits the I/Q data streams to the SDR programs.

Software
-----

This SDR receiver should work with most of the programs that support the HPSDR/Metis communication protocol:

 - [PowerSDR mRX PS](http://openhpsdr.org/wiki/index.php?title=PowerSDR) that can be downloaded from [this link](https://github.com/TAPR/OpenHPSDR-PowerSDR/releases)

 - [QUISK](http://james.ahlstrom.name/quisk) with the `hermes/quisk_conf.py` configuration file

 - [CW Skimmer Server](http://dxatlas.com/skimserver) and [RTTY Skimmer Server](http://dxatlas.com/RttySkimServ)

 - [ghpsdr3-alex](http://napan.ca/ghpsdr3) client-server distributed system

 - [openHPSDR Android Application](https://play.google.com/store/apps/details?id=org.g0orx.openhpsdr) that is described in more details at [this link](http://g0orx.blogspot.be/2015/01/openhpsdr-android-application.html)

 - [Java desktop application](http://g0orx.blogspot.co.uk/2015/04/java-desktop-application-based-on.html) based on openHPSDR Android Application

Getting started
-----

 - Download [SD card image zip file]({{ site.release-image }}) (more details about the SD card image can be found at [this link]({{ "/alpine/" | prepend: site.baseurl }})).
 - Copy the contents of the SD card image zip file to a micro SD card with a MBR/MSDOS partition table and a single FAT32 partition.
 - Optionally, to start the application automatically at boot time, copy its `start.sh` file from `apps/sdr_receiver_hpsdr` to the topmost directory on the SD card.
 - Install the micro SD card in the Red Pitaya board and connect the power.
 - Install and run one of the HPSDR programs.

Running CW Skimmer Server and Reverse Beacon Network Aggregator
-----

 - Install [CW Skimmer Server](http://dxatlas.com/skimserver).
 - Copy [HermesIntf.dll](https://github.com/k3it/HermesIntf/releases) to the CW Skimmer Server program directory (C:\Program Files (x86)\Afreet\SkimSrv).
 - Install [Reverse Beacon Network Aggregator](http://www.reversebeacon.net/pages/Aggregator+34).
 - Start CW Skimmer Server, configure frequencies and your call sign.
 - Start Reverse Beacon Network Aggregator.

Building from source
-----

The installation of the development machine is described at [this link]({{ "/development-machine/" | prepend: site.baseurl }}).

The structure of the source code and of the development chain is described at [this link]({{ "/led-blinker/" | prepend: site.baseurl }}).

Setting up the Vitis and Vivado environment:
{% highlight bash %}
source /opt/Xilinx/Vitis/2019.2/settings64.sh
{% endhighlight %}

Cloning the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
{% endhighlight %}

Building `sdr_receiver_hpsdr.bit`:
{% highlight bash %}
make NAME=sdr_receiver_hpsdr bit
{% endhighlight %}

Building `sdr-receiver-hpsdr`:
{% highlight bash %}
arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -D_GNU_SOURCE projects/sdr_receiver_hpsdr/server/sdr-receiver-hpsdr.c -o sdr-receiver-hpsdr -lm -lpthread
{% endhighlight %}

Building SD card image zip file:
{% highlight bash %}
source helpers/build-all.sh
{% endhighlight %}
