---
layout: page
title: SDR transceiver
permalink: /sdr-transceiver/
---

This is a work in progress...

Hardware
-----

The SDR transceiver consists of two SDR receivers and of two SDR transmitters.

The implementation of the SDR receivers is quite straightforward:

 - An antenna is connected to one of the high-impedance analog inputs.
 - The on-board ADC (125 MS/s sampling frequency, 14-bit resolution) digitizes the RF signal from the antenna.
 - The data coming from the ADC is processed by a in-phase/quadrature (I/Q) digital down-converter (DDC) running on the Red Pitaya's FPGA.

The SDR receiver is described in more details at [this link]({{ "/sdr-receiver/" | prepend: site.baseurl }}).

The SDR transmitters consist of the similar blocks but arranged in an opposite order:

 - The I/Q data is processed by a digital up-converter (DUC) running on the Red Pitaya's FPGA.
 - The on-board DAC (125 MS/s sampling frequency, 14-bit resolution) outputs RF signal.
 - An antenna is connected to one of the analog outputs.

The tunable frequency range covers from 0 Hz to 60 MHz.

The I/Q data rate is configurable and five settings are available: 20, 50, 100, 250 and 500 kSPS.

The basic blocks of the digital down-converters (DDC) and of the digital up-converters (DUC) are shown on the following diagram:

![SDR transceiver]({{ "/img/sdr-transceiver.png" | prepend: site.baseurl }})

The [projects/sdr_transceiver](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver) directory contains four Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver/block_design.tcl), [trx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver/trx.tcl), [rx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver/rx.tcl), [tx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver/tx.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

The [projects/sdr_transceiver/server](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver/server) directory contains the source code of two TCP servers:

  - [sdr-receiver.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver/server/sdr-receiver.c) that receives control commands and transmits the I/Q data stream (up to 2 x 32 bit x 500 kSPS = 30.5 Mbit/s) to the SDR programs
  - [sdr-transmitter.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver/server/sdr-transmitter.c) that receives the I/Q data stream (up to 2 x 32 bit x 500 kSPS = 30.5 Mbit/s) from the SDR programs.

Software
-----

An interface with [GNU Radio](http://gnuradio.org) and [QSDR](http://dl2stg.de/stefan/hiqsdr/qsdr.html) is currently being developed.

Building from source
-----

The installation of the development machine is described at [this link]({{ "/development-machine/" | prepend: site.baseurl }}).

The structure of the source code and of the development chain is described at [this link]({{ "/led-blinker/" | prepend: site.baseurl }}).

Setting up the Vivado environment:
{% highlight bash %}
source /opt/Xilinx/Vivado/2015.2/settings64.sh
source /opt/Xilinx/SDK/2015.2/settings64.sh
{% endhighlight %}

Cloning the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
{% endhighlight %}

Building `sdr_transceiver.bin`:
{% highlight bash %}
make NAME=sdr_transceiver tmp/sdr_transceiver.bit
python scripts/fpga-bit-to-bin.py --flip tmp/sdr_transceiver.bit sdr_transceiver.bin
{% endhighlight %}
