---
layout: page
title: Wideband SDR transceiver
permalink: /sdr-transceiver-wide/
---

Introduction
-----

This version of the Red Pitaya SDR transceiver may be useful for wideband applications.

Hardware
-----

The structure of this version is very similar to the SDR transceiver described at [this link]({{ "/sdr-transceiver/" | prepend: site.baseurl }}). The two main differences are:

 - only one RX and one TX channel,
 - higher sample rates (up to 2500 kSPS).

The basic blocks of the digital down-converter (DDC) and of the digital up-converter (DUC) are shown on the following diagram:

![Wideband SDR transceiver]({{ "/img/sdr-transceiver-wide.png" | prepend: site.baseurl }})

The [projects/sdr_transceiver_wide](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_wide) directory contains four Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_wide/block_design.tcl), [trx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_wide/trx.tcl), [rx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_wide/rx.tcl), [tx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_wide/tx.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

Software
-----

The [projects/sdr_transceiver_wide/server](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_wide/server) directory contains the source code of the TCP server ([sdr-transceiver-wide.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_wide/server/sdr-transceiver-wide.c)) that receives control commands and transmits/receives the I/Q data streams (up to 2 x 32 bit x 2500 kSPS = 152 Mbit/s) to/from the SDR programs.

The [projects/sdr_transceiver_wide/gnuradio](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_wide/gnuradio) directory contains [GNU Radio](http://gnuradio.org) blocks and a few flow graph configurations for [GNU Radio Companion](http://gnuradio.org/redmine/projects/gnuradio/wiki/GNURadioCompanion).

The [projects/sdr_transceiver_wide/gnuradio](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_wide/gnuradio) directory contains [GNU Radio](http://gnuradio.org) blocks and an example flow graph configuration for [GNU Radio Companion](http://gnuradio.org/redmine/projects/gnuradio/wiki/GNURadioCompanion).

Getting started
-----

 - Requirements:
   - Computer running Ubuntu 14.04 or Debian 8.
   - Wired or wireless Ethernet connection between the computer and the Red Pitaya board.
 - Connect an antenna to the IN1 connector on the Red Pitaya board.
 - Download customized [SD card image zip file](https://www.dropbox.com/sh/5fy49wae6xwxa8a/AAD99HKLx4gE1OmNLsjAidWAa/sdr/ecosystem-0.95-1-6deb253-sdr-transceiver-wide.zip?dl=1).
 - Copy the content of the SD card image zip file to an SD card.
 - Insert the SD card in Red Pitaya and connect the power.
 - Install [GNU Radio](http://gnuradio.org):
{% highlight bash %}
sudo apt-get install gnuradio python-numpy python-gtk2
{% endhighlight %}
 - Clone the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
{% endhighlight %}
 - Run [GNU Radio Companion](http://gnuradio.org/redmine/projects/gnuradio/wiki/GNURadioCompanion) and open an example flow graph:
{% highlight bash %}
cd red-pitaya-notes/projects/sdr_transceiver_wide/gnuradio
export GRC_BLOCKS_PATH=.
gnuradio-companion trx_wide_template.grc
{% endhighlight %}

Building from source
-----

The installation of the development machine is described at [this link]({{ "/development-machine/" | prepend: site.baseurl }}).

The structure of the source code and of the development chain is described at [this link]({{ "/led-blinker/" | prepend: site.baseurl }}).

Setting up the Vivado environment:
{% highlight bash %}
source /opt/Xilinx/Vivado/2016.4/settings64.sh
{% endhighlight %}

Cloning the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
{% endhighlight %}

Building `sdr_transceiver_wide.bit`:
{% highlight bash %}
make NAME=sdr_transceiver_wide tmp/sdr_transceiver_wide.bit
{% endhighlight %}

Building `sdr-transceiver`:
{% highlight bash %}
arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard projects/sdr_transceiver_wide/server/sdr-transceiver-wide.c -o sdr-transceiver -lm -lpthread
{% endhighlight %}

Building SD card image zip file:
{% highlight bash %}
source helpers/sdr-transceiver-wide-ecosystem.sh
{% endhighlight %}
