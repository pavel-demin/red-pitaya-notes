---
layout: page
title: SDR transceiver compatible with HPSDR
permalink: /sdr-transceiver-hpsdr/
---

Introduction
-----

The [High Performance Software Defined Radio](http://openhpsdr.org) (HPSDR) project is an open source hardware and software project that develops a modular Software Defined Radio (SDR) for use by radio amateurs and short wave listeners.

This version of the Red Pitaya SDR transceiver makes it usable with the software developed by the HPSDR project and other SDR programs that support the HPSDR/Metis communication protocol.

This SDR transceiver emulates a HPSDR transceiver with one [Metis](http://openhpsdr.org/metis.php) network interface module, two [Mercury](http://openhpsdr.org/mercury.php) receivers and one [Pennylane ](http://openhpsdr.org/penny.php) transmitter.

The HPSDR/Metis communication protocol is described in the following documents:

 - [Metis - How it works](http://svn.tapr.org/repos_sdr_hpsdr/trunk/Metis/Documentation/Metis-%20How%20it%20works_V1.33.pdf)

 - [HPSDR - USB Data Protocol](http://svn.tapr.org/repos_sdr_hpsdr/trunk/Documentation/USB_protocol_V1.58.doc)

Hardware
-----

The implementation of this SDR transceiver is similar to the previous version of the SDR transceiver that is described in more details at [this link]({{ "/sdr-transceiver/" | prepend: site.baseurl }}).

The main problem in emulating the HPSDR hardware with Red Pitaya is that the Red Pitaya ADC sample rate is 125 MSPS and the HPSDR ADC sample rate is 122.88 MSPS.

To address this problem, this version contains a set of FIR filters for fractional sample rate conversion.

The resulting I/Q data rate is configurable and four settings are available: 48, 96, 192, 384 kSPS.

The tunable frequency range covers from 0 Hz to 61.44 MHz.

The basic blocks of the digital down-converters (DDC) and of the digital up-converter (DUC) are shown on the following diagram:

![SDR transceiver compatible with HPSDR ]({{ "/img/sdr-transceiver-hpsdr.png" | prepend: site.baseurl }})

The [projects/sdr_transceiver_hpsdr](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_hpsdr) directory contains three Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_hpsdr/block_design.tcl), [rx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_hpsdr/rx.tcl), [tx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_hpsdr/tx.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

The [projects/sdr_transceiver_hpsdr/filters](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_hpsdr/filters) directory contains the source code of the [R](http://www.r-project.org) scripts used to calculate the coefficients of the FIR filters.

The [projects/sdr_transceiver_hpsdr/server](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_hpsdr/server) directory contains the source code of the UDP server ([sdr-transceiver-hpsdr.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_hpsdr/server/sdr-transceiver-hpsdr.c)) that receives control commands and transmits/receives the I/Q data streams to/from the SDR programs.

External connections
-----

 - input for RX1 is connected to IN1
 - inputs for RX2 and RX3 are connected to IN2
 - output for TX is connected to OUT1
 - output for a RX/TX switch control (PTT-out) is connected to pin DIO0_P of the [extension connector E1](http://wiki.redpitaya.com/index.php?title=Extension_connectors#Extension_connector_E1)
 - output for a pre-amplifier/attenuator control is connected to pin DIO1_P of the [extension connector E1](http://wiki.redpitaya.com/index.php?title=Extension_connectors#Extension_connector_E1) (this pin is controlled by the first ATT combo-box in [PowerSDR mRX PS](http://openhpsdr.org/wiki/index.php?title=PowerSDR))
 - inputs for PTT, DASH and DOT are connected to the pins DIO0_N, DIO1_N and DIO2_N of the [extension connector E1](http://wiki.redpitaya.com/index.php?title=Extension_connectors#Extension_connector_E1)

Software
-----

This SDR transceiver should work with most of the programs that support the HPSDR/Metis communication protocol:

 - [PowerSDR mRX PS](http://openhpsdr.org/wiki/index.php?title=PowerSDR) that can be downloaded from [this link](http://svn.tapr.org/repos_sdr_hpsdr/trunk/W5WC/PowerSDR_Installers) and its skins can be downloaded from [this link](
http://svn.tapr.org/repos_sdr_hpsdr/trunk/W5WC/OpenHPSDR_Skins)

 - [QUISK](http://james.ahlstrom.name/quisk) with the `hermes/quisk_conf.py` configuration file

 - [Ham VNA](http://dxatlas.com/HamVNA) vector network analyzer

 - [ghpsdr3-alex](http://napan.ca/ghpsdr3) client-server distributed system

 - [openHPSDR Android Application](https://play.google.com/store/apps/details?id=org.g0orx.openhpsdr) that is described in more details at [this link](http://g0orx.blogspot.be/2015/01/openhpsdr-android-application.html)

 - [Java desktop application](http://g0orx.blogspot.co.uk/2015/04/java-desktop-application-based-on.html) based on openHPSDR Android Application

Getting started
-----

 - Download customized [SD card image zip file](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/SDR/ecosystem-0.92-65-35575ed-sdr-transceiver-hpsdr.zip).
 - Copy the content of the SD card image zip file to an SD card.
 - Insert the SD card in Red Pitaya and connect the power.
 - Install and run one of the HPSDR programs.

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

Building `sdr_transceiver_hpsdr.bit`:
{% highlight bash %}
make NAME=sdr_transceiver_hpsdr tmp/sdr_transceiver_hpsdr.bit
{% endhighlight %}

Building `sdr-transceiver-hpsdr`:
{% highlight bash %}
arm-xilinx-linux-gnueabi-gcc projects/sdr_transceiver_hpsdr/server/sdr-transceiver-hpsdr.c -o sdr-transceiver-hpsdr -D_GNU_SOURCE -lm -lpthread -static
{% endhighlight %}

Building `boot.bin`, `devicetree.dtb` and `uImage`:
{% highlight bash %}
make NAME=red_pitaya_0_92 all
{% endhighlight %}

Building SD card image zip file:
{% highlight bash %}
source scripts/sdr-transceiver-hpsdr-ecosystem.sh
{% endhighlight %}