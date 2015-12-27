---
layout: page
title: Multichannel Pulse Height Analyzer
permalink: /mcpha/
---

Interesting links
-----

Some interesting links on spectrum analysis:

 - [Spectrum Analysis Introduction](http://www.canberra.com/literature/fundamental-principles/pdf/Spectrum-Analysis.pdf)

 - [Gamma Spectrometry Software](https://www.youtube.com/watch?v=bBG_m4akFts)

Hardware
-----

The basic blocks of the system are shown on the following diagram:

![Multichannel Pulse Height Analyzer]({{ "/img/mcpha.png" | prepend: site.baseurl }})

The [projects/mcpha](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/mcpha) directory contains four Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/mcpha/block_design.tcl), [hst.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/mcpha/hst.tcl), [osc.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/mcpha/osc.tcl), [gen.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/mcpha/gen.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

Software
-----

The [projects/mcpha/server](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/mcpha/server) directory contains the source code of the TCP server ([mcpha-server.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/mcpha/server/mcpha-server.c)) that receives control commands and transmits the data to the control program running on a remote PC.

The [projects/mcpha/client](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/mcpha/client) directory contains the source code of the control program ([mcpha.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/mcpha/client/mcpha.tcl)).

![MCPHA client]({{ "/img/mcpha-client.png" | prepend: site.baseurl }})

Getting started with MS Windows
-----

 - Requirements:
   - Computer running MS Windows.
   - Wired or wireless Ethernet connection between the computer and the Red Pitaya board.
 - Connect a signal source to the IN1 or IN2 connector on the Red Pitaya board.
 - Download customized [SD card image zip file](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/mcpha/ecosystem-0.92-65-35575ed-mcpha.zip).
 - Copy the content of the SD card image zip file to an SD card.
 - Insert the SD card in Red Pitaya and connect the power.
 - Download and unpack the [control program](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/mcpha/mcpha-win32-20151272.zip).
 - Run the control program.
 - Type in the IP address of the Red Pitaya board and press Connect button.
 - Select Spectrum histogram 1 or Spectrum histogram 2 tab.
 - Adjust amplitude threshold and time of exposure.
 - Press Start button.

Getting started with GNU/Linux
-----

 - Requirements:
   - Computer running Debian 8.
   - Wired or wireless Ethernet connection between the computer and the Red Pitaya board.
 - Connect a signal source to the IN1 or IN2 connector on the Red Pitaya board.
 - Download customized [SD card image zip file](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/mcpha/ecosystem-0.92-65-35575ed-mcpha.zip).
 - Copy the content of the SD card image zip file to an SD card.
 - Insert the SD card in Red Pitaya and connect the power.
 - Install Tcl 8.6 and all the required libraries:
{% highlight bash %}
sudo apt-get install tcl8.6-dev tk8.6-dev blt-dev tk8.6-blt2.5 tcllib
{% endhighlight %}
 - Clone the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
{% endhighlight %}
 - Run the control program:
{% highlight bash %}
cd red-pitaya-notes/projects/mcpha/client
gcc -shared -fPIC -I /usr/include/tcl8.6 mcpha.c -o mcpha.so -lBLT
wish8.6 mcpha.tcl
{% endhighlight %}
 - Type in the IP address of the Red Pitaya board and press Connect button.
 - Select Spectrum histogram 1 or Spectrum histogram 2 tab.
 - Adjust amplitude threshold and time of exposure.
 - Press Start button.

Building from source
-----

The installation of the development machine is described at [this link]({{ "/development-machine/" | prepend: site.baseurl }}).

The structure of the source code and of the development chain is described at [this link]({{ "/led-blinker/" | prepend: site.baseurl }}).

Setting up the Vivado environment:
{% highlight bash %}
source /opt/Xilinx/Vivado/2015.4/settings64.sh
source /opt/Xilinx/SDK/2015.4/settings64.sh
{% endhighlight %}

Cloning the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
{% endhighlight %}

Building `mcpha.bit`:
{% highlight bash %}
make NAME=mcpha tmp/mcpha.bit
{% endhighlight %}

Building `mcpha-server`:
{% highlight bash %}
arm-xilinx-linux-gnueabi-gcc projects/mcpha/server/mcpha-server.c -o mcpha-server -lm -lpthread -static
{% endhighlight %}

Building `boot.bin`, `devicetree.dtb` and `uImage`:
{% highlight bash %}
make NAME=red_pitaya_0_92 all
{% endhighlight %}

Building SD card image zip file:
{% highlight bash %}
source scripts/mcpha-ecosystem.sh
{% endhighlight %}
