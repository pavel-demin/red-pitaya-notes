---
layout: page
title: SDR transceiver compatible with HPSDR
permalink: /sdr-transceiver-hpsdr/
---

Introduction
-----

The [High Performance Software Defined Radio](http://openhpsdr.org) (HPSDR) project is an open source hardware and software project that develops a modular Software Defined Radio (SDR) for use by radio amateurs and short wave listeners.

This version of the Red Pitaya SDR transceiver makes it usable with the software developed by the HPSDR project and other SDR programs that support the HPSDR/Metis communication protocol.

This SDR transceiver emulates a HPSDR transceiver similar to [Hermes](http://openhpsdr.org/hermes.php) with a network interface, two receivers and one transmitter.

The HPSDR/Metis communication protocol is described in the following documents:

 - [Metis - How it works](http://svn.tapr.org/repos_sdr_hpsdr/trunk/Metis/Documentation/Metis-%20How%20it%20works_V1.33.pdf)

 - [HPSDR - USB Data Protocol](http://svn.tapr.org/repos_sdr_hpsdr/trunk/Documentation/USB_protocol_V1.58.doc)

Hardware
-----

The implementation of this SDR transceiver is similar to the previous version of the SDR transceiver that is described in more details at [this link]({{ "/sdr-transceiver/" | prepend: site.baseurl }}).

The main problem in emulating the HPSDR hardware with Red Pitaya is that the Red Pitaya ADC sample rate is 125 MSPS and the HPSDR ADC sample rate is 122.88 MSPS.

To address this problem, this version contains a set of FIR filters for fractional sample rate conversion.

The resulting I/Q data rate is configurable and four settings are available: 48, 96, 192, 384 kSPS.

The tunable frequency range covers from 0 Hz to 61.44 MHz.

This SDR transceiver consists of four digital down-converters (DDC) and one digital up-converter (DUC). The first two digital down-converters are connected to two ADC channels. Two additional digital down-converters are required for the amplifier linearization system. One of them is connected to the second ADC channel and the other one is connected to the output of the digital up-converter.

The basic blocks of the digital down-converters are shown on the following diagram:

![DDC]({{ "/img/sdr-transceiver-hpsdr-ddc.png" | prepend: site.baseurl }})

The digital up-converter consists of similar blocks but arranged in an opposite order:

![DUC]({{ "/img/sdr-transceiver-hpsdr-duc.png" | prepend: site.baseurl }})

The [projects/sdr_transceiver_hpsdr](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_hpsdr) directory contains three Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_hpsdr/block_design.tcl), [rx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_hpsdr/rx.tcl), [tx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_hpsdr/tx.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

The [projects/sdr_transceiver_hpsdr/filters](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_hpsdr/filters) directory contains the source code of the [R](http://www.r-project.org) scripts used to calculate the coefficients of the FIR filters.

The [projects/sdr_transceiver_hpsdr/server](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_hpsdr/server) directory contains the source code of the UDP server ([sdr-transceiver-hpsdr.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_hpsdr/server/sdr-transceiver-hpsdr.c)) that receives control commands and transmits/receives the I/Q data streams to/from the SDR programs.

RF, GPIO and XADC connections
-----

 - input for RX1 is connected to IN1
 - inputs for RX2 and RX3 are connected to IN2
 - output for TX is connected to OUT1
 - output for a RX/TX switch control (PTT-out) is connected to pin DIO0_P of the [extension connector E1](http://redpitaya.readthedocs.io/en/latest/developerGuide/125-14/extent.html#extension-connector-e1)
 - output for a pre-amplifier/attenuator control is connected to pin DIO1_P of the [extension connector E1](http://redpitaya.readthedocs.io/en/latest/developerGuide/125-14/extent.html#extension-connector-e1) (this pin is controlled by the first ATT combo-box in [PowerSDR mRX PS](http://openhpsdr.org/wiki/index.php?title=PowerSDR))
 - outputs for 10 dB and 20 dB attenuators control are connected to the pins DIO2_P - DIO3_P of the [extension connector E1](http://redpitaya.readthedocs.io/en/latest/developerGuide/125-14/extent.html#extension-connector-e1)
 - outputs for Hermes Ctrl pins are connected to the pins DIO4_P - DIO7_P of the [extension connector E1](http://redpitaya.readthedocs.io/en/latest/developerGuide/125-14/extent.html#extension-connector-e1)
 - inputs for PTT, DASH and DOT are connected to the pins DIO0_N, DIO1_N and DIO2_N of the [extension connector E1](http://redpitaya.readthedocs.io/en/latest/developerGuide/125-14/extent.html#extension-connector-e1)
 - slow analog inputs can be used for the forward ([Analog input 0](http://redpitaya.readthedocs.io/en/latest/developerGuide/125-14/extent.html#extension-connector-e2)) and reverse ([Analog input 1](http://redpitaya.readthedocs.io/en/latest/developerGuide/125-14/extent.html#extension-connector-e2)) power measurement

![GPIO connections]({{ "/img/sdr-transceiver-hpsdr-e1-pins.png" | prepend: site.baseurl }})

I2S connections
-----

The I2S interface is sharing pins with the ALEX interface. So, the two can't be used simultaneously. The supported I2S audio codecs are [TLV320AIC23B](http://www.ti.com/product/TLV320AIC23B) and [WM8731](http://www.cirrus.com/en/products/pro/detail/P1307.html). The I2S audio codecs should be clocked with a 12.288 MHz oscillator crystal.

The I2S interface should be connected to the [extension connector E1](http://redpitaya.readthedocs.io/en/latest/developerGuide/125-14/extent.html#extension-connector-e1) as shown on the above diagram. The I2C interface should be connected to the I2C pins of the [extension connector E2](http://redpitaya.readthedocs.io/en/latest/developerGuide/125-14/extent.html#extension-connector-e2).

ALEX connections
-----
The [ALEX module](http://openhpsdr.org/alex.php) can be connected to the pins DIO4_N (Serial Data), DIO5_N (Clock), DIO6_N (RX Strobe) and DIO7_N (TX Strobe) of the [extension connector E1](http://redpitaya.readthedocs.io/en/latest/developerGuide/125-14/extent.html#extension-connector-e1).
The board and the protocol are described in the [ALEX manual](http://www.tapr.org/pdf/ALEX_Manual_V1_0.pdf).

The HPSDR signals sent to the [TPIC6B595](http://www.ti.com/product/TPIC6B595) chips are shown on the following diagram:

![ALEX connections]({{ "/img/sdr-transceiver-hpsdr-alex-interface.png" | prepend: site.baseurl }})

I2C connections
-----

This interface is designed by Peter DC2PD. The [sdr-transceiver-hpsdr.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_hpsdr/server/sdr-transceiver-hpsdr.c) server communicates with one or two [PCA9555](http://www.ti.com/product/PCA9555) chips connected to the I2C pins of the [extension connector E2](http://redpitaya.readthedocs.io/en/latest/developerGuide/125-14/extent.html#extension-connector-e2).

HPSDR signals sent to the [PCA9555](http://www.ti.com/product/PCA9555) chip at address 0 (0x20):

PCA9555 pins | HPSDR signals
------------ | -------------
P00 - P06    | Open Collector Outputs on Penelope or Hermes
P07 - P10    | Attenuator (00 = 0dB, 01 = 10dB, 10 = 20dB, 11 = 30dB)
P11 - P12    | Rx Antenna (00 = none, 01 = Rx1, 10 = Rx2, 11 = XV)
P13 - P14    | Tx Relay (00 = Tx1, 01= Tx2, 10 = Tx3)

HPSDR signals sent to the [PCA9555](http://www.ti.com/product/PCA9555) chip at address 1 (0x21):

PCA9555 pins | HPSDR signals
------------ | -------------
P00          | select 13MHz HPF (0 = disable, 1 = enable)
P01          | select 20MHz HPF (0 = disable, 1 = enable)
P02          | select 9.5MHz HPF (0 = disable, 1 = enable)
P03          | select 6.5MHz HPF (0 = disable, 1 = enable)
P04          | select 1.5MHz HPF (0 = disable, 1 = enable)
P05          | bypass all HPFs (0 = disable, 1 = enable)
P06          | 6M low noise amplifier (0 = disable, 1 = enable)
P07          | disable T/R relay (0 = enable, 1 = disable)
P10          | select 30/20m LPF (0 = disable, 1 = enable)
P11          | select 60/40m LPF (0 = disable, 1 = enable)
P12          | select 80m LPF (0 = disable, 1 = enable)
P13          | select 160m LPF (0 = disable, 1 = enable)
P14          | select 6m LPF (0 = disable, 1 = enable)
P15          | select 12/10m LPF (0 = disable, 1 = enable)
P16          | select 17/15m LPF (0 = disable, 1 = enable)

Signals sent to the [PCA9555](http://www.ti.com/product/PCA9555) chip at address 3 (0x23):

PCA9555 pins | HPSDR signals
------------ | -------------
P00 - P03    | BCD code for Rx1 band
P04 - P07    | BCD code for Rx2 band
P10          | Tx frequency (0 if Tx freq. = Rx1 freq., 1 if Tx freq. = Rx2 freq.)
P11 - P12    | ATT1
P13 - P14    | ATT2
P15          | disable T/R relay (0 = enable, 1 = disable)
P16          | bypass all HPFs (0 = disable, 1 = enable)

More information about the I2C interface can be found at [this link](https://www.dropbox.com/sh/5fy49wae6xwxa8a/AABuxJW6dpV50d6QPvUQNCUza/sdr/Hermes_and_Alex_outputs.pdf?dl=1).

Software
-----

This SDR transceiver should work with most of the programs that support the HPSDR/Metis communication protocol:

 - [PowerSDR mRX PS](http://openhpsdr.org/wiki/index.php?title=PowerSDR) that can be downloaded from [this link](https://github.com/TAPR/OpenHPSDR-PowerSDR/releases)

 - [QUISK](http://james.ahlstrom.name/quisk) with the `hermes/quisk_conf.py` configuration file

 - [ghpsdr3-alex](http://napan.ca/ghpsdr3) client-server distributed system

 - [openHPSDR Android Application](https://play.google.com/store/apps/details?id=org.g0orx.openhpsdr) that is described in more details at [this link](http://g0orx.blogspot.be/2015/01/openhpsdr-android-application.html)

 - [Java desktop application](http://g0orx.blogspot.co.uk/2015/04/java-desktop-application-based-on.html) based on openHPSDR Android Application

Getting started
-----

 - Download [SD card image zip file]({{ site.release-image }}) (more details about the SD card image can be found at [this link]({{ "/alpine/" | prepend: site.baseurl }})).
 - Copy the content of the SD card image zip file to an SD card.
 - Optionally, to start the application automatically at boot time, copy its `start.sh` file from `apps/sdr_transceiver_hpsdr` to the topmost directory on the SD card.
 - Insert the SD card in Red Pitaya and connect the power.
 - Install and run one of the HPSDR programs.

Configuring inputs and outputs
-----

The `sdr-transceiver-hpsdr` program running on the Red Pitaya board expects five command line arguments:
```
sdr-transceiver-hpsdr 1 2 2 1 2
```

The first three arguments are for the receivers (RX1, RX2, RX3), where 1 corresponds to IN1 and 2 corresponds to IN2.

The last two arguments are for the outputs (OUT1, OUT2), where 1 corresponds to the TX signal and 2 corresponds to the envelope signal.

For example, to send the TX signal to OUT2, the corresponding line in [start.sh](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_hpsdr/app/start.sh#L9) should be edited and the last argument should be set to 1:
```
sdr-transceiver-hpsdr 1 2 2 1 1
```

Amplifier linearization
-----

[PowerSDR mRX PS](http://openhpsdr.org/wiki/index.php?title=PowerSDR) includes an amplifier linearization system called [PureSignal](http://svn.tapr.org/repos_sdr_hpsdr/trunk/W5WC/PowerSDR_HPSDR_mRX_PS/bin/Release/PureSignal.pdf). The following screenshots show what settings should be adjusted when using it with Red Pitaya. To access the "Calibration Information" panel press Ctrl+Alt+i. The attenuated feedback signal from the amplifier should be connected to IN2.

![PowerSDR Hardware Config]({{ "/img/powersdr-hardware.png" | prepend: site.baseurl }})

![PowerSDR Linearity]({{ "/img/powersdr-linearity.png" | prepend: site.baseurl }})

The following spectra illustrate how the amplifier linearization works with the Red Pitaya output (OUT1) connected to the Red Pitaya input (IN2) with a 50 Ohm termination.

![PureSignal off]({{ "/img/puresignal-off.png" | prepend: site.baseurl }})
![PureSignal on]({{ "/img/puresignal-on.png" | prepend: site.baseurl }})

CW functionality
-----

The CW keyer can be used with a straight or iambic key connected to the pins DIO1_N and DIO2_N of the [extension connector E1](http://redpitaya.readthedocs.io/en/latest/developerGuide/125-14/extent.html#extension-connector-e1). The CW signal is generated when one of the CW modes is selected in [PowerSDR mRX PS](http://openhpsdr.org/wiki/index.php?title=PowerSDR) and the pins DIO1_N and DIO2_N are connected to GND.

The ramp generator is programmable. The default ramp's shape is the step response of the 4-term Blackman-Harris window. It's inspired by the ["CW Shaping in DSP Software"](https://github.com/pavel-demin/red-pitaya-notes/files/403696/cw-shaping-in-dsp.pdf) article appeared in the May/June, 2006 issue of QEX.

The measured delay between the key press and the start of the RF signal is about 2 ms. The 10%-90% rise time of the signal is about 3.5 ms.

![CW signal]({{ "/img/cw-signal.png" | prepend: site.baseurl }})

The following figure shows the spectrum of the CW signal keyed at 50 WPM.

![CW spectrum]({{ "/img/cw-spectrum.png" | prepend: site.baseurl }})

Building from source
-----

The installation of the development machine is described at [this link]({{ "/development-machine/" | prepend: site.baseurl }}).

The structure of the source code and of the development chain is described at [this link]({{ "/led-blinker/" | prepend: site.baseurl }}).

Setting up the Vivado environment:
{% highlight bash %}
source /opt/Xilinx/Vivado/2018.1/settings64.sh
{% endhighlight %}

Cloning the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
{% endhighlight %}

Building `sdr_transceiver_hpsdr.bit`:
{% highlight bash %}
make NAME=sdr_transceiver_hpsdr bit
{% endhighlight %}

Building `sdr-transceiver-hpsdr`:
{% highlight bash %}
arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -D_GNU_SOURCE projects/sdr_transceiver_hpsdr/server/sdr-transceiver-hpsdr.c -o sdr-transceiver-hpsdr -lm -lpthread
{% endhighlight %}

Building SD card image zip file:
{% highlight bash %}
source helpers/build-all.sh
{% endhighlight %}
