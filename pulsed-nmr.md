---
layout: page
title: Pulsed Nuclear Magnetic Resonance
permalink: /pulsed-nmr/
---

This is a work in progress...

Interesting links
-----

Some interesting links on pulsed nuclear magnetic resonance:

 - [Pulsed NMR at UW](http://courses.washington.edu/phys431/PNMR/pulsed_nmr.html)

 - [Pulsed NMR at MSU](https://www.pa.msu.edu/courses/2016spring/PHY451/Experiments/pulsed_nmr.html)

 - [The Basics of NMR](https://www.cis.rit.edu/htbooks/nmr) by Joseph P. Hornak

Short description
-----

The system consists of one in-phase/quadrature (I/Q) digital down-converter (DDC) and of one pulse generator.

The tunable frequency range covers from 0 Hz to 60 MHz.

The I/Q data rate is configurable and five settings are available: 25, 50, 250, 500, 2500 kSPS.

Hardware
-----

The basic blocks of the system are shown on the following diagram:

![Pulsed NMR]({{ "/img/pulsed-nmr.png" | prepend: site.baseurl }})

The [projects/pulsed_nmr](https://github.com/pavel-demin/red-pitaya-notes/tree/develop/projects/pulsed_nmr) directory contains three Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/develop/projects/pulsed_nmr/block_design.tcl), [rx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/develop/projects/pulsed_nmr/rx.tcl), [tx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/develop/projects/pulsed_nmr/tx.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

Software
-----

The [projects/pulsed_nmr/server](https://github.com/pavel-demin/red-pitaya-notes/tree/develop/projects/pulsed_nmr/server) directory contains the source code of the TCP server ([pulsed-nmr.c](https://github.com/pavel-demin/red-pitaya-notes/blob/develop/projects/pulsed_nmr/server/pulsed-nmr.c)) that receives control commands and transmits the I/Q data streams (up to 2 x 32 bit x 2500 kSPS = 152 Mbit/s) to the control program running on a remote PC.

The [projects/pulsed_nmr/client](https://github.com/pavel-demin/red-pitaya-notes/tree/develop/projects/pulsed_nmr/client) directory contains the source code of the control program ([pulsed_nmr.py](https://github.com/pavel-demin/red-pitaya-notes/blob/develop/projects/pulsed_nmr/client/pulsed_nmr.py)).

![Pulsed NMR client]({{ "/img/pulsed-nmr-client.png" | prepend: site.baseurl }})

Getting started with GNU/Linux
-----

 - Download customized [SD card image zip file](https://www.dropbox.com/sh/5fy49wae6xwxa8a/AABG0Ki6nvRlxKm0q7Yb07z3a/pulsed_nmr/ecosystem-0.95-1-6deb253-pulsed-nmr.zip?dl=1).
 - Take a freshly unpacked micro SD card factory-formatted with the FAT32 file system.
 - Copy the contents of the SD card image zip file to the micro SD card.
 - Install the micro SD card in the Red Pitaya board and connect the power.
 - Install required Python libraries:
{% highlight bash %}
sudo apt-get install python3-dev python3-pip python3-numpy python3-pyqt5 libfreetype6-dev
sudo pip3 install matplotlib
{% endhighlight %}
 - Clone the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
{% endhighlight %}
 - Run the control program:
{% highlight bash %}
cd red-pitaya-notes/projects/pulsed_nmr/client
python3 pulsed_nmr.py
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

Building `pulsed_nmr.bit`:
{% highlight bash %}
make NAME=pulsed_nmr bit
{% endhighlight %}

Building `pulsed-nmr`:
{% highlight bash %}
arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard projects/pulsed_nmr/server/pulsed-nmr.c -o pulsed-nmr -lm
{% endhighlight %}

Building SD card image zip file:
{% highlight bash %}
source helpers/pulsed-nmr-ecosystem.sh
{% endhighlight %}
