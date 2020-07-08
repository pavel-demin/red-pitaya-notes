---
layout: page
title: Pulsed NMR system
permalink: /pulsed-nmr-122-88/
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

The I/Q data rate is configurable and seven settings are available: 20, 40, 80, 160, 320, 640, 1280 kSPS.

Hardware
-----

The basic blocks of the system are shown on the following diagram:

![Pulsed NMR]({{ "/img/pulsed-nmr-122-88.png" | prepend: site.baseurl }})

The [projects/pulsed_nmr_122_88](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/pulsed_nmr_122_88) directory contains three Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/pulsed_nmr_122_88/block_design.tcl), [rx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/pulsed_nmr_122_88/rx.tcl), [tx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/pulsed_nmr_122_88/tx.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

Software
-----

The [projects/pulsed_nmr_122_88/server](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/pulsed_nmr_122_88/server) directory contains the source code of the TCP server ([pulsed-nmr.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/pulsed_nmr_122_88/server/pulsed-nmr.c)) that receives control commands and transmits the I/Q data streams (up to 4 x 32 bit x 1280 kSPS = 156 Mbit/s) to the control program running on a remote PC.

The [projects/pulsed_nmr_122_88/client](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/pulsed_nmr_122_88/client) directory contains the source code of the control program ([pulsed_nmr.py](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/pulsed_nmr_122_88/client/pulsed_nmr.py)).

![Pulsed NMR client]({{ "/img/pulsed-nmr-client.png" | prepend: site.baseurl }})

Getting started with GNU/Linux
-----

 - Download [SD card image zip file]({{ site.release-image }}) (more details about the SD card image can be found at [this link]({{ "/alpine/" | prepend: site.baseurl }})).
 - Copy the contents of the SD card image zip file to a micro SD card.
 - Optionally, to start the application automatically at boot time, copy its `start.sh` file from `apps/pulsed_nmr_122_88` to the topmost directory on the SD card.
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
cd red-pitaya-notes/projects/pulsed_nmr_122_88/client
python3 pulsed_nmr.py
{% endhighlight %}

Building from source
-----

The installation of the development machine is described at [this link]({{ "/development-machine/" | prepend: site.baseurl }}).

The structure of the source code and of the development chain is described at [this link]({{ "/led-blinker/" | prepend: site.baseurl }}).

Setting up the Vitis and Vivado environment:
{% highlight bash %}
source /opt/Xilinx/Vitis/2020.1/settings64.sh
{% endhighlight %}

Cloning the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
{% endhighlight %}

Building `pulsed_nmr_122_88.bit`:
{% highlight bash %}
make NAME=pulsed_nmr_122_88 PART=xc7z020clg400-1 bit
{% endhighlight %}

Building `pulsed-nmr`:
{% highlight bash %}
arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard projects/pulsed_nmr_122_88/server/pulsed-nmr.c -o pulsed-nmr -lm
{% endhighlight %}

Building SD card image zip file:
{% highlight bash %}
source helpers/build-all.sh
{% endhighlight %}
