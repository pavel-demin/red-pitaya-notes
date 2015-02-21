---
layout: page
title: SDR receiver
permalink: /sdr-receiver/
---

Digital Down Converter
-----

Red Pitaya has all the components of a Software Defined Radio (SDR). In order to test its performance, I've implemented a Digital Down Converter (DDC) following [Xilinx application note 1113](http://www.xilinx.com/support/documentation/application_notes/xapp1113.pdf). The basic blocks of this converter are shown on the following diagram:

![SDR receiver]({{ "/img/sdr-receiver.png" | prepend: site.baseurl }})

I/Q demodulator is implemented using the [CORDIC algorithm](http://www.xilinx.com/products/intellectual-property/cordic.html). [CIC filter](http://www.xilinx.com/products/intellectual-property/cic_compiler.html) is used to decrease the data rate by a factor of 625. [FIR filter](http://www.xilinx.com/products/intellectual-property/fir_compiler.html) compensates for the drop in the CIC frequency response, filters out frequencies above 50 kHz and reduces the data rate by a factor of two.

FIR filter coefficients are calculated with the following code in [R](http://www.r-project.org):
{% highlight R %}
library(signal)

# CIC filter parameters
R <- 625                       # Decimation factor
M <- 1                         # Differential delay
N <- 6                         # Number of stages

Fo <- 0.23                     # Pass band edge; 46 kHz

# fir2 parameters
k <- kaiserord(c(Fo, Fo+0.01), c(1, 0), 1/(2^16), 1)
L <- k$n                       # Filter order
Beta <- k$beta                 # Kaiser window parameter

# FIR filter design using fir2
s <- 0.001                     # Step size
fp <- seq(0.0, Fo, by=s)       # Pass band frequency samples
fs <- seq(Fo+0.01, 0.5, by=s)  # Stop band frequency samples
f <- c(fp, fs)*2               # Normalized frequency samples; 0<=f<=1

Mp <- matrix(1, 1, length(fp)) # Pass band response; Mp[1]=1
Mp[-1] <- abs(M*R*sin(pi*fp[-1]/R)/sin(pi*M*fp[-1]))^N
Mf <- c(Mp, matrix(0, 1, length(fs)))

h <- fir2(L, f, Mf, window=kaiser(L+1, Beta))

# Print filter coefficients
paste(as.character(h), collapse=", ")
{% endhighlight %}

To get an idea of the combined (CIC and FIR) filter response, the following figure shows a 128k FFT display from the SDR# program when Red Pitaya inputs are not connected to anything and the SDR# filtering is switched off:

![Filter response]({{ "/img/no-signal.png" | prepend: site.baseurl }})

The [projects/sdr_receiver](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver) directory contains one Tcl file [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_receiver/block_design.tcl) that instantiates, configures and interconnects all the needed IP cores.

The [projects/sdr_receiver/multicast](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver/multicast) directory contains one C program [sdr-receiver.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_receiver/multicast/sdr-receiver.c). This program transmits the I/Q data stream (2 x 32 bit x 100 kSPS = 6.1 Mbit/s) via IP multicast and receives multicast packets containing control commands and frequency of the sine and cosine waves used for the I/Q demodulation. By default, the multicast address is `239.0.0.39`, the port number for the control commands and for the frequency information is `1001`, the port number for the I/Q data stream is `1002`.

User interface
-----

The I/Q data coming from Red Pitaya can be analyzed and processed by a SDR program such as [SDR#](http://sdrsharp.com/#download) or [HDSDR](http://www.hdsdr.de/).

The SDR programs are communicating with the SDR radio hardware through an External Input Output Dynamic Link Library (ExtIO-DLL). The detailed specifications of this interface and the source code examples can be found at the following links:

 - [Winrad - specifications for the external I/O DLL](http://www.winrad.org/bin/Winrad_Extio.pdf)
 - [HDSDR FAQ](http://www.hdsdr.de/faq.html)

Based on the [example ExtIO DLL](http://hdsdr.de/download/ExtIO/ExtIO_Demo_101.zip), I've developed a simple ExtIO plug-in for the Red Pitaya SDR receiver. The [projects/sdr_receiver/ExtIO_RedPitaya](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver/ExtIO_RedPitaya) directory contains the source code of this plug-in.

The ExtIO plug-in can be built from the source code with [Microsoft Visual C++ 2008 Express Edition](http://go.microsoft.com/?linkid=7729279).

A pre-built ExtIO plug-in for the Red Pitaya SDR receiver can be downloaded from [this link](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/ExtIO_RedPitaya.dll).

For both SDR# and HDSDR, the `ExtIO_RedPitaya.dll` file should be copied to the directory where the program is installed and the program will recognize it automatically at start-up.

Antenna
-----

Inspired by the "Wideband active loop antenna" article appeared in the January, 2001 issue of Elektor Electronics, I've build my antenna using 4 wire telephone cable (9 m, 4 x 0.2 mm<sup>2</sup>). A schematic and picture of the antenna connected to Red Pitaya is shown in the following figure:

![Antenna schematic]({{ "/img/antenna-schematic.png" | prepend: site.baseurl }}) ![Antenna picture]({{ "/img/antenna-picture.jpg" | prepend: site.baseurl }})

With this antenna I can receive some MW and SW broadcast stations.

Screen shot and audio sample
-----

Signal from a 300 kW broadcast MW transmitter, 25 km from the transmitter:

![Strong signal]({{ "/img/strong-signal.png" | prepend: site.baseurl }})

[Audio sample](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/strong-signal.wav)

Getting started
-----

 - Requirements:
   - Computer running MS Windows.
   - Wired Ethernet connection between the computer and the Red Pitaya board.
 - Connect an antenna to the IN2 connector on the Red Pitaya board.
 - Download [pre-built SD card image](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/red-pitaya-sdr-receiver-20150221.zip).
 - Write it to a SD card using [Win32 Disk Imager](http://sourceforge.net/projects/win32diskimager/).
 - Insert the newly created SD card in Red Pitaya and connect the power.
 - Download and install [SDR#](http://sdrsharp.com/#download) or [HDSDR](http://www.hdsdr.de/).
 - Download [pre-built ExtIO plug-in](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/ExtIO_RedPitaya.dll) for SDR# and HDSDR.
 - Copy `ExtIO_RedPitaya.dll` into the SDR# or HDSDR installation directory.
 - Start SDR# or HDSDR.
 - Select Red Pitaya SDR from the Source list in SDR# or from the Options [F7] &rarr; Select Input menu in HDSDR.
 - Press Play icon in SDR# or press Start [F2] button in HDSDR.

SD card image
-----

The SD card image size is 512 MB, so it should fit on any SD card starting from 1 GB.

To write the image to a SD card, the `dd` command-line utility can be used on GNU/Linux and Mac OS X or [Win32 Disk Imager](http://sourceforge.net/projects/win32diskimager/) can be used on MS Windows.

The default user name is `root` and the default password is `changeme`.

The `sdr-receiver` program is started automatically at boot. If needed, it can be started and stopped with the following commands:
{% highlight bash %}
start sdr-receiver
stop sdr-receiver
{% endhighlight %}

To use the full size of a SD card, the partitions can be resized on running Red Pitaya with the following commands:
{% highlight bash %}
# delete second partition
echo -e "d\n2\nw" | fdisk /dev/mmcblk0
# recreate partition
parted -s /dev/mmcblk0 mkpart primary ext4 16MB 100%
# resize partition
resize2fs /dev/mmcblk0p2
{% endhighlight %}

Building from source
-----

The installation of the development machine is described at [this link]({{ "/development-machine/" | prepend: site.baseurl }}).

The structure of the source code and of the development chain is described at [this link]({{ "/led-blinker/" | prepend: site.baseurl }}).

Setting up the Vivado environment:
{% highlight bash %}
source /opt/Xilinx/Vivado/2014.3.1/settings64.sh
source /opt/Xilinx/SDK/2014.3.1/settings64.sh
{% endhighlight %}

Cloning the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
{% endhighlight %}

Building `u-boot.elf`, `boot.bin` and `devicetree.dtb`:
{% highlight bash %}
make NAME=sdr_receiver all
{% endhighlight %}

Building `sdr-receiver`:
{% highlight bash %}
arm-linux-gnueabihf-gcc projects/sdr_receiver/multicast/sdr-receiver.c -o sdr-receiver -lm
{% endhighlight %}

Building a bootable SD card image:
{% highlight bash %}
sudo sh scripts/image.sh red-pitaya-sdr-receiver.img
{% endhighlight %}

Copying `sdr-receiver` to SD card image:
{% highlight bash %}
device=`sudo losetup -f`
sudo losetup $device red-pitaya-sdr-receiver.img
sudo mkdir /tmp/ROOT
sudo mount /dev/`lsblk -lno NAME $device | sed '3!d'` /tmp/ROOT
sudo cp sdr-receiver /tmp/ROOT/usr/bin/
sudo cp projects/sdr_receiver/multicast/sdr-receiver.conf /tmp/ROOT/etc/init/
sudo umount /tmp/ROOT
sudo rmdir /tmp/ROOT
sudo losetup -d $device
{% endhighlight %}
