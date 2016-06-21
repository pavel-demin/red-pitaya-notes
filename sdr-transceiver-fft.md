---
layout: page
title: SDR transceiver with FFT
permalink: /sdr-transceiver-fft/
---

This is a work in progress...

![SDR transceiver with FFT]({{ "/img/sdr-transceiver-fft.png" | prepend: site.baseurl }})

The [projects/sdr_transceiver_fft](https://github.com/pavel-demin/red-pitaya-notes/tree/develop/projects/sdr_transceiver_fft) directory contains four Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/develop/projects/sdr_transceiver_fft/block_design.tcl), [rx_0.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/develop/projects/sdr_transceiver_fft/rx_0.tcl), [sp_0.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/develop/projects/sdr_transceiver_fft/sp_0.tcl), [tx_0.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/develop/projects/sdr_transceiver_fft/tx_0.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

Software
-----

A software called [MiniTRX](https://github.com/pavel-demin/MiniTRX) is currently being developed. This software consists of two components:

 - server that runs on the Red Pitaya board and processes all the I/Q data
 - client that runs on a computer, receives the audio and spectrum data from the server and sends the audio data to the server for transmission

The server relies on the following libraries:

 - [FFTW-ARM](http://www.vesperix.com/arm/fftw-arm/)
 - [WDSP](http://openhpsdr.org/videos.php)
 - [libsamplerate](http://www.mega-nerd.com/SRC/)
 - [Qt](http://www.qt.io/)

The client is mainly based on the [Qt](http://www.qt.io/) library.

The [WebSocket](http://doc.qt.io/qt-5/qtwebsockets-index.html) protocol is used for the communication between the server and the client.

The client GUI is written in the [QML](http://doc.qt.io/qt-5/qtqml-index.html) language.

Building from source
-----

The installation of the development machine is described at [this link]({{ "/development-machine/" | prepend: site.baseurl }}).

The structure of the source code and of the development chain is described at [this link]({{ "/led-blinker/" | prepend: site.baseurl }}).

Setting up the Vivado environment:
{% highlight bash %}
source /opt/Xilinx/Vivado/2016.2/settings64.sh
source /opt/Xilinx/SDK/2016.2/settings64.sh
{% endhighlight %}

Cloning the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
{% endhighlight %}

Building `sdr_transceiver_fft.bin`:
{% highlight bash %}
make NAME=sdr_transceiver_fft tmp/sdr_transceiver_fft.bit
python scripts/fpga-bit-to-bin.py --flip tmp/sdr_transceiver_fft.bit sdr_transceiver_fft.bin
{% endhighlight %}
