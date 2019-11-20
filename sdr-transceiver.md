---
layout: page
title: SDR transceiver
permalink: /sdr-transceiver/
---

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

The I/Q data rate is configurable and five settings are available: 20, 50, 100, 250, 500 and 1250 kSPS.

The basic blocks of the digital down-converters (DDC) and of the digital up-converters (DUC) are shown on the following diagram:

![SDR transceiver]({{ "/img/sdr-transceiver.png" | prepend: site.baseurl }})

The [projects/sdr_transceiver](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver) directory contains four Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver/block_design.tcl), [trx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver/trx.tcl), [rx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver/rx.tcl), [tx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver/tx.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

Software
-----

The [projects/sdr_transceiver/server](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver/server) directory contains the source code of the TCP server ([sdr-transceiver.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver/server/sdr-transceiver.c)) that receives control commands and transmits/receives the I/Q data streams (up to 2 x 32 bit x 1250 kSPS = 76.3 Mbit/s) to/from the SDR programs.

The [projects/sdr_transceiver/ExtIO_RedPitaya_TRX](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver/ExtIO_RedPitaya_TRX) directory contains the source code of the ExtIO plug-in.

The [projects/sdr_transceiver/gnuradio](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver/gnuradio) directory contains [GNU Radio](http://gnuradio.org) blocks and a few flow graph configurations for [GNU Radio Companion](https://wiki.gnuradio.org/index.php/GNURadioCompanion).

Getting started with GNU Radio
-----

 - Connect an antenna to the IN1 connector on the Red Pitaya board.
 - Download [SD card image zip file]({{ site.release-image }}) (more details about the SD card image can be found at [this link]({{ "/alpine/" | prepend: site.baseurl }})).
 - Copy the content of the SD card image zip file to an SD card.
 - Optionally, to start the application automatically at boot time, copy its `start.sh` file from `apps/sdr_transceiver` to the topmost directory on the SD card.
 - Insert the SD card in Red Pitaya and connect the power.
 - Install [GNU Radio](http://gnuradio.org):
{% highlight bash %}
sudo apt-get install gnuradio python-numpy python-gtk2
{% endhighlight %}
 - Clone the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
{% endhighlight %}
 - Run [GNU Radio Companion](http://gnuradio.org/redmine/projects/gnuradio/wiki/GNURadioCompanion) and open AM transceiver flow graph:
{% highlight bash %}
cd red-pitaya-notes/projects/sdr_transceiver/gnuradio
export GRC_BLOCKS_PATH=.
gnuradio-companion trx_am.grc
{% endhighlight %}

Getting started with SDR# and HDSDR
-----

 - Connect an antenna to the IN1 connector on the Red Pitaya board.
 - Download [SD card image zip file]({{ site.release-image }}) (more details about the SD card image can be found at [this link]({{ "/alpine/" | prepend: site.baseurl }})).
 - Copy the content of the SD card image zip file to an SD card.
 - Optionally, to start the application automatically at boot time, copy its `start.sh` file from `apps/sdr_transceiver` to the topmost directory on the SD card.
 - Insert the SD card in Red Pitaya and connect the power.
 - Download and install [SDR#](https://www.dropbox.com/sh/5fy49wae6xwxa8a/AAAdAcU238cppWziK4xPRIADa/sdr/sdrsharp_v1.0.0.1361_with_plugins.zip?dl=1) or [HDSDR](http://www.hdsdr.de/).
 - Download and install [Microsoft Visual C++ 2015 Redistributable](https://www.microsoft.com/en-us/download/details.aspx?id=53587).
 - Download [pre-built ExtIO plug-in](https://www.dropbox.com/sh/5fy49wae6xwxa8a/AAABy8xWrv4wFFbkYfmVU6DGa/sdr/ExtIO_RedPitaya_TRX.dll?dl=1) for SDR# and HDSDR.
 - Copy `ExtIO_RedPitaya_TRX.dll` into the SDR# or HDSDR installation directory.
 - Start SDR# or HDSDR.
 - Select Red Pitaya SDR TRX from the Source list in SDR# or from the Options [F7] &rarr; Select Input menu in HDSDR.
 - Press Configure icon in SDR# or press ExtIO button in HDSDR, then type in the IP address of the Red Pitaya board and close the configuration window.
 - Press Play icon in SDR# or press Start [F2] button in HDSDR.

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

Building `sdr_transceiver.bit`:
{% highlight bash %}
make NAME=sdr_transceiver bit
{% endhighlight %}

Building `sdr-transceiver`:
{% highlight bash %}
arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard projects/sdr_transceiver/server/sdr-transceiver.c -o sdr-transceiver -lm -lpthread
{% endhighlight %}

Building SD card image zip file:
{% highlight bash %}
source helpers/build-all.sh
{% endhighlight %}
