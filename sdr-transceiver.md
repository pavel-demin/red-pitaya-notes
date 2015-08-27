---
layout: page
title: SDR transceiver
permalink: /sdr-transceiver/
---

This is a work in progress...

Hardware
-----

The SDR transceiver consists of the SDR receiver and of the SDR transmitter.

The implementation of the SDR receiver is quite straightforward:

 - An antenna is connected to one of the high-impedance analog inputs.
 - The on-board ADC (125 MS/s sampling frequency, 14-bit resolution) digitizes the RF signal from the antenna.
 - The data coming from the ADC is processed by a in-phase/quadrature (I/Q) digital down-converter (DDC) running on the Red Pitaya's FPGA.

The SDR receiver is described in more details at [this link]({{ "/sdr-receiver/" | prepend: site.baseurl }}).

The SDR transmitter consists of the similar blocks but arranged in an opposite order:

 - The I/Q data is processed by a digital up-converter (DUC) running on the Red Pitaya's FPGA.
 - The on-board DAC (125 MS/s sampling frequency, 14-bit resolution) outputs RF signal.
 - An antenna is connected to one of the analog outputs.

The tunable frequency range covers from 0 Hz to 50 MHz.

The receiver and transmitter I/Q data rates are fixed to 20 kSPS.

The spectrum display I/Q data rate is configurable and four settings are available: 50, 100, 250 and 500 kSPS.

The basic blocks of the digital down-converter (DDC) and of the digital up-converter (DUC) are shown on the following diagram:

![SDR transceiver]({{ "/img/sdr-transceiver.png" | prepend: site.baseurl }})

The [projects/sdr_transceiver](https://github.com/pavel-demin/red-pitaya-notes/tree/develop/projects/sdr_transceiver) directory contains three Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/develop/projects/sdr_transceiver/block_design.tcl), [rx_0.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/develop/projects/sdr_transceiver/rx_0.tcl), [tx_0.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/develop/projects/sdr_transceiver/tx_0.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

Software
-----

An interface with GNU Radio is currently being developed.

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
