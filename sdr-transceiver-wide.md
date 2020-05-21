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

The [projects/sdr_transceiver_wide/gnuradio](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_wide/gnuradio) directory contains [GNU Radio](http://gnuradio.org) blocks and a few flow graph configurations for [GNU Radio Companion](https://wiki.gnuradio.org/index.php/GNURadioCompanion).

The [projects/sdr_transceiver_wide/gnuradio](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_wide/gnuradio) directory contains [GNU Radio](http://gnuradio.org) blocks and an example flow graph configuration for [GNU Radio Companion](https://wiki.gnuradio.org/index.php/GNURadioCompanion).

Transmitting and receiving complex baseband signals
-----

To transmit and to receive complex baseband signals via two outputs (OUT1 and OUT2) and two inputs (IN1 and IN2), the central frequency should be set to 0 and the IQ bitmask should be set to 3.

![IQ configuration]({{ "/img/iq-grc.png" | prepend: site.baseurl }})

Here is a picture of a complex 1 kHz cosine waveform as seen by an oscilloscope connected to OUT1 and OUT2:

![IQ waveform]({{ "/img/iq-osc.png" | prepend: site.baseurl }})

Getting started
-----

 - Connect an antenna to the IN1 connector on the Red Pitaya board.
 - Download [SD card image zip file]({{ site.release-image }}) (more details about the SD card image can be found at [this link]({{ "/alpine/" | prepend: site.baseurl }})).
 - Take a freshly unpacked micro SD card factory-formatted with the FAT32 file system.
 - Copy the contents of the SD card image zip file to the micro SD card.
 - Optionally, to start the application automatically at boot time, copy its `start.sh` file from `apps/sdr_transceiver_wide` to the topmost directory on the SD card.
 - Install the micro SD card in the Red Pitaya board and connect the power.
 - Install [GNU Radio](http://gnuradio.org):
{% highlight bash %}
sudo apt-get install gnuradio python-numpy python-gtk2
{% endhighlight %}
 - Clone the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
{% endhighlight %}
 - Run [GNU Radio Companion](https://wiki.gnuradio.org/index.php/GNURadioCompanion) and open an example flow graph:
{% highlight bash %}
cd red-pitaya-notes/projects/sdr_transceiver_wide/gnuradio
export GRC_BLOCKS_PATH=.
gnuradio-companion trx_wide_template.grc
{% endhighlight %}

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

Building `sdr_transceiver_wide.bit`:
{% highlight bash %}
make NAME=sdr_transceiver_wide bit
{% endhighlight %}

Building `sdr-transceiver`:
{% highlight bash %}
arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard projects/sdr_transceiver_wide/server/sdr-transceiver-wide.c -o sdr-transceiver-wide -lm -lpthread
{% endhighlight %}

Building SD card image zip file:
{% highlight bash %}
source helpers/build-all.sh
{% endhighlight %}
