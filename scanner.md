---
layout: page
title: Scanning system
permalink: /scanner/
---

Introduction
-----

This configuration may be useful for building a galvanometer controller for an optical scanner or a scanning coil controller for a scanning electron microscope.

Interesting links
-----

Some interesting links on scanning and imaging techniques:

 - [Scanning and Image Reconstruction Techniques in Confocal Laser Scanning Microscopy](http://www.intechopen.com/books/laser-scanning-theory-and-applications/scanning-and-image-reconstruction-techniques-in-confocal-laser-scanning-microscopy)

 - [Spectral Imaging: Active hyperspectral sensing and imaging for remote spectroscopy applications](http://www.laserfocusworld.com/articles/print/volume-49/issue-11/features/spectral-imaging-active-hyperspectral-sensing-and-imaging-for-remote-spectroscopy-applications.html)

Hardware
-----

The system outputs the line scan signal to OUT1 and the raster scan signal to OUT2. The trigger and S&H pulses are available from the pins DIO0_N and DIO1_N on the E1 connector. The analog signals corresponding to each pixel are input to IN1 and IN2.

The basic blocks of the system are shown on the following diagram:

![Scanner]({{ "/img/scanner.png" | prepend: site.baseurl }})

The [projects/scanner](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/scanner) directory contains one Tcl file [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/scanner/block_design.tcl) that instantiates, configures and interconnects all the needed IP cores.

Software
-----

The [projects/scanner/server](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/scanner/server) directory contains the source code of the TCP server ([scanner.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/scanner/server/scanner.c)) that receives control commands and transmits the data to the control program running on a remote PC.

The [projects/scanner/client](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/scanner/client) directory contains the source code of the control program ([scanner.py](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/scanner/client/scanner.py)).

![Scanner client]({{ "/img/scanner-client.png" | prepend: site.baseurl }})

Getting started with MS Windows
-----

 - Download customized [SD card image zip file]({{ site.scanner-image }}).
 - Copy the content of the SD card image zip file to an SD card.
 - Insert the SD card in Red Pitaya and connect the power.
 - Download and install [Python 3.4](https://www.python.org/ftp/python/3.4.4/python-3.4.4.msi).
 - Download and install [PyQt 5.5](https://sourceforge.net/projects/pyqt/files/PyQt5/PyQt-5.5.1/PyQt5-5.5.1-gpl-Py3.4-Qt5.5.1-x32.exe/download).
 - Start a command prompt using the `cmd.exe` program and run the `pip` command to install `matplotlib`:
{% highlight winbatch %}
C:\Python34\python.exe -m pip install --upgrade pip
C:\Python34\Scripts\pip.exe install numpy==1.15.4 matplotlib
{% endhighlight %}
 - Download [scanner.py](https://raw.githubusercontent.com/pavel-demin/red-pitaya-notes/master/projects/scanner/client/scanner.py) and [scanner.ui](https://raw.githubusercontent.com/pavel-demin/red-pitaya-notes/master/projects/scanner/client/scanner.ui).
 - Start a command prompt using the `cmd.exe` program and run the control program:
{% highlight winbatch %}
C:\Python34\pythonw.exe scanner.py
{% endhighlight %}
 - Type in the IP address of the Red Pitaya board and press Connect button.
 - Adjust trigger and S&H pulses and number of samples per pixel.
 - Press Scan button.

Getting started with GNU/Linux
-----

 - Download customized [SD card image zip file]({{ site.scanner-image }}).
 - Copy the content of the SD card image zip file to an SD card.
 - Insert the SD card in Red Pitaya and connect the power.
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
cd red-pitaya-notes/projects/scanner/client
python3 scanner.py
{% endhighlight %}
 - Type in the IP address of the Red Pitaya board and press Connect button.
 - Adjust trigger and S&H pulses and number of samples per pixel.
 - Press Scan button.

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

Building `scanner.bit`:
{% highlight bash %}
make NAME=scanner bit
{% endhighlight %}

Building `scanner`:
{% highlight bash %}
arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard projects/scanner/server/scanner.c -o scanner -lm
{% endhighlight %}

Building SD card image zip file:
{% highlight bash %}
source helpers/build-scanner.sh
{% endhighlight %}
