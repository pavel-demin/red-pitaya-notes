---
layout: page
title: SDR receiver
permalink: /sdr-receiver/
---

Digital Down Converter
-----

Red Pitaya has all the components of a Software Defined Radio (SDR). In order to test its performance, I've implemented a Digital Down Converter following [Xilinx application note 1113](http://www.xilinx.com/support/documentation/application_notes/xapp1113.pdf). The basic blocks of this converter are shown on the following diagram:

![SDR receiver]({{ "/img/sdr-receiver.png" | prepend: site.baseurl }})

I/Q demodulator is implemented using the CORDIC algorithm. CIC filter is used to decrease the data rate by a factor of 625. FIR filter compensates for the drop in the CIC frequency response, filters out frequencies above 50 kHz and reduces the data rate by a factor of two.

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

The [projects/sdr_receiver](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver) directory contains one Tcl file [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_receiver/block_design.tcl) that instantiates, configures and interconnects all the needed IP cores.

The [projects/sdr_receiver/multicast](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver/multicast) directory contains two C programs  [sdr_freq.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_receiver/multicast/sdr_freq.c) and [sdr_data.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_receiver/multicast/sdr_data.c).

The I/Q data stream (2 x 32 bit x 100 kSPS = 6.4 Mbit/s) is transmitted by the `sdr_data` program via IP multicast. The `sdr_freq` program receives multicast packets containing frequency of the sine and cosine waves used for the I/Q demodulation. By default, the multicast address is `239.0.0.39`, the port number for the frequency information is `1001` and the port number for the I/Q data stream is `1002`.

User interface
-----

The I/Q data coming from Red Pitaya can be analyzed and processed by a SDR program like [SDR#](http://sdrsharp.com/#download) or [HDSDR](http://www.hdsdr.de/).

The SDR programs are communicating with the SDR radio hardware through an External Input Output Dynamic Link Library (ExtIO-DLL). The detailed specifications of this interface and the source code examples can be found at the following links:

 - [Winrad - specifications for the external I/O DLL](http://www.winrad.org/bin/Winrad_Extio.pdf)
 - [HDSDR FAQ](http://www.hdsdr.de/faq.html)

Based on the [example ExtIO DLL](http://hdsdr.de/download/ExtIO/ExtIO_Demo_101.zip), I've developed a simple ExtIO plug-in for the Red Pitaya SDR receiver. The [projects/sdr_receiver/ExtIO_RedPitaya](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver/ExtIO_RedPitaya) directory contains the source code of this plug-in.

The ExtIO plug-in can be built from the source code with [Microsoft Visual C++ 2008 Express Edition](http://go.microsoft.com/?linkid=7729279).

A pre-built ExtIO plug-in for the Red Pitaya SDR receiver can be downloaded from [this link](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/ExtIO_RedPitaya.dll).

For both SDR# and HDSDR, the `ExtIO_RedPitaya.dll` file should be copied to the directory where the program is installed and the program will recognize it automatically at start-up.

SD card image
-----

A pre-built SD card image can be downloaded from [this link](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/red-pitaya-sdr-receiver-20150218.zip).

To write the image to SD card, the `dd` command can be used on Linux and Mac OS X and [Win32 Disk Imager](http://sourceforge.net/projects/win32diskimager/) can be used on MS Windows.

The user name is `root` and the default password is `changeme`.

Starting the two multicast programs on Red Pitaya:
{% highlight bash %}
./sdr_data &
./sdr_freq &
{% endhighlight %}

Antenna
-----

Inspired by the "Wideband active loop antenna" article appeared in the January, 2001 issue of Elektor Electronics, I've build my antenna using 4 pair telephone cable (20 m, 4 x 0.5 mm<sup>2</sup>). The schematic is shown on the following diagram:

![Antenna]({{ "/img/antenna.png" | prepend: site.baseurl }})

With this antenna I can receive some broadcast stations.

Screen shot and audio sample
-----

Signal from a 300 kW broadcast MW transmitter, 25 km from the transmitter:

![Strong signal]({{ "/img/strong-signal.png" | prepend: site.baseurl }})

[Audio sample](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/strong-signal.wav)

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

Building `sdr_freq` and `sdr_data`:
{% highlight bash %}
arm-linux-gnueabihf-gcc projects/sdr_receiver/multicast/sdr_freq.c -o sdr_freq -lm
arm-linux-gnueabihf-gcc projects/sdr_receiver/multicast/sdr_data.c -o sdr_data -lm
{% endhighlight %}

Building a bootable SD card image:
{% highlight bash %}
sudo sh scripts/image.sh red-pitaya-sdr-receiver.img
{% endhighlight %}

Resizing SD card partitions on running Red Pitaya:
{% highlight bash %}
# delete second partition
echo -e "d\n2\nw" | fdisk /dev/mmcblk0
# recreate partition
parted -s /dev/mmcblk0 mkpart primary ext4 16MB 100%
# resize partition
resize2fs /dev/mmcblk0p2
{% endhighlight %}
