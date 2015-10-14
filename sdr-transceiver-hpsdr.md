---
layout: page
title: SDR transceiver compatible with HPSDR 
permalink: /sdr-transceiver-hpsdr/
---

This is a work in progress...

![SDR transceiver compatible with HPSDR ]({{ "/img/sdr-transceiver-hpsdr.png" | prepend: site.baseurl }})

The [projects/sdr_transceiver_hpsdr](https://github.com/pavel-demin/red-pitaya-notes/tree/develop/projects/sdr_transceiver_hpsdr) directory contains three Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/develop/projects/sdr_transceiver_hpsdr/block_design.tcl), [rx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/develop/projects/sdr_transceiver_hpsdr/rx.tcl), [tx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/develop/projects/sdr_transceiver_hpsdr/tx.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

The [projects/sdr_transceiver_hpsdr/server](https://github.com/pavel-demin/red-pitaya-notes/tree/develop/projects/sdr_transceiver_hpsdr/server) directory contains the source code of the TCP server ([sdr-transceiver-hpsdr.c](https://github.com/pavel-demin/red-pitaya-notes/blob/develop/projects/sdr_transceiver_hpsdr/server/sdr-transceiver-hpsdr.c)) that receives control commands and transmits/receives the I/Q data streams (up to 2 x 32 bit x 384 kSPS = 23.4 Mbit/s) to/from the SDR programs.

Software
-----

The [projects/sdr_transceiver_hpsdr/gnuradio](https://github.com/pavel-demin/red-pitaya-notes/tree/develop/projects/sdr_transceiver_hpsdr/gnuradio) directory contains [GNU Radio](http://gnuradio.org) blocks and a few flow graph configurations for [GNU Radio Companion](http://gnuradio.org/redmine/projects/gnuradio/wiki/GNURadioCompanion).

An interface with [HPSDR](http://openhpsdr.org) is currently [being developed](https://github.com/pavel-demin/red-pitaya-notes/tree/develop/projects/sdr_transceiver_hpsdr/hpsdr).

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
arm-xilinx-linux-gnueabi-gcc projects/sdr_transceiver_hpsdr/server/sdr-transceiver-hpsdr.c -o sdr-transceiver-hpsdr -lm -lpthread -static
{% endhighlight %}
