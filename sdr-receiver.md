---
layout: page
title: SDR receiver
permalink: /sdr-receiver/
---

Interesting links
-----

Some interesting links on digital signal processing and software defined radio:

 - [dspGuru - Digital Signal Processing Articles](http://www.dspguru.com/dsp/articles)

 - [ARRL - Software Defined Radio](http://www.arrl.org/software-defined-radio)

 - [GNU Radio - Suggested Reading](http://gnuradio.org/redmine/projects/gnuradio/wiki/SuggestedReading)

Short description
-----

The implementation of the SDR receiver is quite straightforward:

 - An antenna is connected to one of the high-impedance analog inputs.
 - The on-board ADC (125 MS/s sampling frequency, 14-bit resolution) digitizes the RF signal from the antenna.
 - The data coming from the ADC is processed by a in-phase/quadrature (I/Q) digital down-converter (DDC) running on the Red Pitaya's FPGA.
 - The I/Q data is transmitted via TCP to the SDR programs such as SDR# and HDSDR.

The tunable frequency range covers from 0 Hz to 50 MHz.

The I/Q data rate is configurable and four settings are available: 50, 100, 250 and 500 kSPS.

Digital down-converter
-----

The basic blocks of the digital down-converter (DDC) are shown on the following diagram:

![SDR receiver]({{ "/img/sdr-receiver.png" | prepend: site.baseurl }})

The in-phase/quadrature (I/Q) demodulator is implemented using the [CORDIC algorithm](http://www.xilinx.com/products/intellectual-property/cordic.html). [CIC filter](http://www.xilinx.com/products/intellectual-property/cic_compiler.html) is used to decrease the data rate by a configurable factor within the range from 125 to 1250. [FIR filter](http://www.xilinx.com/products/intellectual-property/fir_compiler.html) compensates for the drop in the CIC frequency response, filters out high frequencies and reduces the data rate by a factor of two.

FIR filter coefficients are calculated with the following code in [R](http://www.r-project.org):
{% highlight R %}
library(signal)

# CIC filter parameters
R <- 125                       # Decimation factor
M <- 1                         # Differential delay
N <- 6                         # Number of stages

Fo <- 0.22                     # Pass band edge; 220 kHz

# fir2 parameters
k <- kaiserord(c(Fo, Fo+0.02), c(1, 0), 1/(2^16), 1)
L <- k$n                       # Filter order
Beta <- k$beta                 # Kaiser window parameter

# FIR filter design using fir2
s <- 0.001                     # Step size
fp <- seq(0.0, Fo, by=s)       # Pass band frequency samples
fs <- seq(Fo+0.02, 0.5, by=s)  # Stop band frequency samples
f <- c(fp, fs)*2               # Normalized frequency samples; 0<=f<=1

Mp <- matrix(1, 1, length(fp)) # Pass band response; Mp[1]=1
Mp[-1] <- abs(M*R*sin(pi*fp[-1]/R)/sin(pi*M*fp[-1]))^N
Mf <- c(Mp, matrix(0, 1, length(fs)))

h <- fir2(L, f, Mf, window=kaiser(L+1, Beta))

# Print filter coefficients
paste(as.character(h), collapse=", ")
{% endhighlight %}

To get an idea of the combined (CIC and FIR) filter response, the following figure shows a 256k FFT display from the SDR# program when Red Pitaya inputs are not connected to anything:

![Filter response]({{ "/img/no-signal.png" | prepend: site.baseurl }})

The [projects/sdr_receiver](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver) directory contains one Tcl file [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_receiver/block_design.tcl) that instantiates, configures and interconnects all the needed IP cores.

The [projects/sdr_receiver/server](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver/server) directory contains the source code of the TCP server ([sdr-receiver.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_receiver/server/sdr-receiver.c)) that transmits the I/Q data stream (up to 2 x 32 bit x 500 kSPS = 30.5 Mbit/s) to the SDR programs and receives commands to configure the decimation rate and the frequency of the sine and cosine waves used for the I/Q demodulation.

User interface
-----

The I/Q data coming from Red Pitaya can be analyzed and processed by a SDR program such as [SDR#](http://sdrsharp.com/#download) or [HDSDR](http://www.hdsdr.de/).

The SDR programs are communicating with the SDR radio hardware through an External Input Output Dynamic Link Library (ExtIO-DLL). The detailed specifications of this interface and the source code examples can be found at the following links:

 - [Winrad - specifications for the external I/O DLL](http://www.winrad.org/bin/Winrad_Extio.pdf)
 - [HDSDR FAQ](http://www.hdsdr.de/faq.html)

Based on the [example ExtIO DLL](http://hdsdr.de/download/ExtIO/ExtIO_Demo_101.zip), I've developed a simple ExtIO plug-in for the Red Pitaya SDR receiver. The [projects/sdr_receiver/ExtIO_RedPitaya](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver/ExtIO_RedPitaya) directory contains the source code of this plug-in.

The ExtIO plug-in can be built from the source code with [Microsoft Visual C++ 2008 Express Edition](http://go.microsoft.com/?linkid=7729279).

A pre-built ExtIO plug-in for the Red Pitaya SDR receiver can be downloaded from [this link](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/SDR/ExtIO_RedPitaya.dll).

For both SDR# and HDSDR, the `ExtIO_RedPitaya.dll` file should be copied to the directory where the program is installed and the program will recognize it automatically at start-up.

Antenna
-----

Inspired by the "Wideband active loop antenna" article appeared in the January, 2000 issue of Elektor Electronics, I've built my antenna using 4 wire telephone cable (9 m, 4 x 0.2 mm<sup>2</sup>). A schematic and picture of the antenna connected to Red Pitaya is shown in the following figure:

![Antenna schematic]({{ "/img/antenna-schematic.png" | prepend: site.baseurl }}) ![Antenna picture]({{ "/img/antenna-picture.jpg" | prepend: site.baseurl }})

With this antenna I can receive some MW and SW broadcast stations.

Screen shot and audio sample
-----

Signal from a 300 kW broadcast MW transmitter, 25 km from the transmitter:

![Strong signal]({{ "/img/strong-signal.png" | prepend: site.baseurl }})

[Audio sample](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/SDR/strong-signal.wav)

Getting started
-----

 - Requirements:
   - Computer running MS Windows.
   - Wired or wireless Ethernet connection between the computer and the Red Pitaya board.
 - Connect an antenna to the IN2 connector on the Red Pitaya board.
 - Download customized [SD card image zip file](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/SDR/ecosystem-0.92-65-35575ed-sdr-receiver.zip).
 - Copy the content of the SD card image zip file to an SD card.
 - Insert the SD card in Red Pitaya and connect the power.
 - Download and install [SDR#](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/SDR/sdrsharp_v1.0.0.1361_with_plugins.zip) or [HDSDR](http://www.hdsdr.de/).
 - Download [pre-built ExtIO plug-in](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/SDR/ExtIO_RedPitaya.dll) for SDR# and HDSDR.
 - Copy `ExtIO_RedPitaya.dll` into the SDR# or HDSDR installation directory.
 - Start SDR# or HDSDR.
 - Select Red Pitaya SDR from the Source list in SDR# or from the Options [F7] &rarr; Select Input menu in HDSDR.
 - Press Configure icon in SDR# or press ExtIO button in HDSDR, then type in the IP address of the Red Pitaya board and close the configuration window.
 - Press Play icon in SDR# or press Start [F2] button in HDSDR.

Building from source
-----

The installation of the development machine is described at [this link]({{ "/development-machine/" | prepend: site.baseurl }}).

The structure of the source code and of the development chain is described at [this link]({{ "/led-blinker/" | prepend: site.baseurl }}).

Setting up the Vivado environment:
{% highlight bash %}
source /opt/Xilinx/Vivado/2016.1/settings64.sh
source /opt/Xilinx/SDK/2016.1/settings64.sh
{% endhighlight %}

Cloning the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
{% endhighlight %}

Building `sdr_receiver.bit`:
{% highlight bash %}
make NAME=sdr_receiver tmp/sdr_receiver.bit
{% endhighlight %}

Building `sdr-receiver`:
{% highlight bash %}
arm-linux-gnueabihf-gcc projects/sdr_receiver/server/sdr-receiver.c -o sdr-receiver -lm -static
{% endhighlight %}

Building SD card image zip file:
{% highlight bash %}
source scripts/sdr-receiver-ecosystem.sh
{% endhighlight %}
