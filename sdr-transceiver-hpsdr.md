---
layout: page
title: SDR transceiver compatible with HPSDR
permalink: /sdr-transceiver-hpsdr/
---

Hardware
-----

This SDR transceiver emulates a [HPSDR](http://openhpsdr.org) transceiver with one [Metis](http://openhpsdr.org/metis.php) network interface module, two [Mercury](http://openhpsdr.org/mercury.php) receivers and one [Pennylane ](http://openhpsdr.org/penny.php) transmitter.

The implementation of the SDR receivers is quite straightforward:

 - An antenna is connected to one of the high-impedance analog inputs.
 - The on-board ADC (125 MS/s sampling frequency, 14-bit resolution) digitizes the RF signal from the antenna.
 - The data coming from the ADC is processed by a in-phase/quadrature (I/Q) digital down-converter (DDC) running on the Red Pitaya's FPGA.

The SDR receiver is described in more details at [this link]({{ "/sdr-receiver/" | prepend: site.baseurl }}).

The SDR transmitter consists of the similar blocks but arranged in an opposite order:

 - The I/Q data is processed by a digital up-converter (DUC) running on the Red Pitaya's FPGA.
 - The on-board DAC (125 MS/s sampling frequency, 14-bit resolution) outputs RF signal.
 - An antenna is connected to one of the analog outputs.

The tunable frequency range covers from 0 Hz to 61.44 MHz.

The I/Q data rate is configurable and four settings are available: 48, 96, 192, 384 kSPS.

The basic blocks of the digital down-converters (DDC) and of the digital up-converter (DUC) are shown on the following diagram:

![SDR transceiver compatible with HPSDR ]({{ "/img/sdr-transceiver-hpsdr.png" | prepend: site.baseurl }})

The [projects/sdr_transceiver_hpsdr](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_hpsdr) directory contains three Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_hpsdr/block_design.tcl), [rx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_hpsdr/rx.tcl), [tx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_hpsdr/tx.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

The [projects/sdr_transceiver_hpsdr/filters](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_hpsdr/filters) directory contains the source code of the [R](http://www.r-project.org) scripts used to calculate the coefficients of the FIR filters.

The [projects/sdr_transceiver_hpsdr/server](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_hpsdr/server) directory contains the source code of the UDP server ([sdr-transceiver-hpsdr.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_hpsdr/server/sdr-transceiver-hpsdr.c)) that receives control commands and transmits/receives the I/Q data streams (up to 2 x 32 bit x 384 kSPS = 23.4 Mbit/s) to/from the SDR programs.

Software
-----

This SDR transceiver should work with most of the programs that support the [HPSDR](http://openhpsdr.org)/[Metis](http://openhpsdr.org/metis.php) protocol:

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
 - Install and run one of the [HPSDR](http://openhpsdr.org) programs.

Building from source
-----

The installation of the development machine is described at [this link]({{ "/development-machine/" | prepend: site.baseurl }}).

The structure of the source code and of the development chain is described at [this link]({{ "/led-blinker/" | prepend: site.baseurl }}).

Setting up the Vivado environment:
{% highlight bash %}
source /opt/Xilinx/Vivado/2015.3/settings64.sh
source /opt/Xilinx/SDK/2015.3/settings64.sh
{% endhighlight %}

Cloning the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
{% endhighlight %}

Building `sdr_transceiver_hpsdr.bin`:
{% highlight bash %}
make NAME=sdr_transceiver_hpsdr tmp/sdr_transceiver_hpsdr.bit
python scripts/fpga-bit-to-bin.py --flip tmp/sdr_transceiver_hpsdr.bit sdr_transceiver_hpsdr.bin
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