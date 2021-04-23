---
layout: page
title: Embedded SDR transceiver
permalink: /sdr-transceiver-emb-122-88/
---

Introduction
-----

This version of the Red Pitaya SDR transceiver may be useful for building a small standalone SDR transceiver with all the SDR algorithms running on the on-board CPU of the Red Pitaya board.

Hardware
-----

The basic blocks of the digital down-converters (DDC) and of the digital up-converters (DUC) are shown on the following diagram:

![Embedded SDR transceiver]({{ "/img/sdr-transceiver-emb-122-88.png" | prepend: site.baseurl }})

The [projects/sdr_transceiver_emb_122_88](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_emb_122_88) directory contains four Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_emb_122_88/block_design.tcl), [rx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_emb_122_88/rx.tcl), [sp.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_emb_122_88/sp.tcl), [tx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_emb_122_88/tx.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

Software
-----

Some examples of software can be found at the following links:

 - [DiscoRedTRX](https://github.com/ted051/DiscoRedTRX),
 - [DiscoRedTRX](https://github.com/pavel-demin/DiscoRedTRX) (old version),
 - [projects/sdr_transceiver_emb_122_88/app](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_emb_122_88/app).

Getting started
-----

 - Download [SD card image zip file]({{ site.release-image }}) (more details about the SD card image can be found at [this link]({{ "/alpine/" | prepend: site.baseurl }})).
 - Copy the contents of the SD card image zip file to a micro SD card.
 - Optionally, to start the application automatically at boot time, copy its `start.sh` file from `apps/sdr_transceiver_emb_122_88` to the topmost directory on the SD card.
 - Install the micro SD card in the Red Pitaya board and connect the power.

Building from source
-----

The installation of the development machine is described at [this link]({{ "/development-machine/" | prepend: site.baseurl }}).

The structure of the source code and of the development chain is described at [this link]({{ "/led-blinker/" | prepend: site.baseurl }}).

Setting up the Vitis and Vivado environment:
{% highlight bash %}
source /opt/Xilinx/Vitis/2020.2/settings64.sh
{% endhighlight %}

Cloning the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
{% endhighlight %}

Building `sdr_transceiver_emb_122_88.bit`:
{% highlight bash %}
make NAME=sdr_transceiver_emb_122_88 PART=xc7z020clg400-1 bit
{% endhighlight %}

Building SD card image zip file:
{% highlight bash %}
source helpers/build-all.sh
{% endhighlight %}
