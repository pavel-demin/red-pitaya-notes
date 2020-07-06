---
layout: page
title: Multiband WSPR transceiver
permalink: /sdr-transceiver-wspr-122-88/
---

Interesting links
-----

Some interesting links on the Weak Signal Propagation Reporter (WSPR) protocol:

 - [WSPR home page](http://physics.princeton.edu/pulsar/k1jt/wspr.html)
 - [WSPR 3.0 User's Guide](http://physics.princeton.edu/pulsar/k1jt/WSPR_3.0_User.pdf)
 - [WSPRnet](http://wsprnet.org)
 - [WSPRnet map](http://wsprnet.org/drupal/wsprnet/map)
 - [WSPRnet protocol](http://wsprnet.org/automate.txt)
 - [WSPRlive](https://wsprlive.net)

Short description
-----

This project implements a standalone multiband WSPR transceiver with all the WSPR signal processing done by STEMlab SDR in the following way:

 - simultaneously record WPSR signals from sixteen bands
 - use FPGA for all the conversions needed to produce .c2 files (complex 32-bit floating-point data at 375 samples per second)
 - use on-board CPU to process the .c2 files with the [WSPR decoder](https://github.com/pavel-demin/wsprd)
 - upload decoded data to [wsprnet.org](http://wsprnet.org)

With this configuration, it is enough to connect STEMlab SDR to an antenna and to a network. After switching STEMlab SDR on, it will automatically start operating as a WSPR receiver.

The transmitter part is disabled by default and should be enabled manually.

Hardware
-----

The FPGA configuration consists of sixteen identical digital down-converters (DDC). Their structure is shown on the following diagram:

![WSPR receiver]({{ "/img/sdr-receiver-wspr-122-88.png" | prepend: site.baseurl }})

The DDC output contains complex 32-bit floating-point data at 375 samples per second and is directly compatible with the [WSPR decoder](https://github.com/pavel-demin/wsprd).

The [projects/sdr_transceiver_wspr_122_88](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_wspr_122_88) directory contains three Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_wspr_122_88/block_design.tcl), [rx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_wspr_122_88/rx.tcl) and [tx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_wspr_122_88/tx.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

Software
-----

The [write-c2-files.c](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_wspr_122_88/app/write-c2-files.c) program accumulates 42000 samples at 375 samples per second for each of the sixteen bands and saves the samples to sixteen .c2 files.

The recorded .c2 files are processed with the [WSPR decoder](https://github.com/pavel-demin/wsprd).

The decoded data are uploaded to [wsprnet.org](http://wsprnet.org) using [curl](https://curl.haxx.se).

The [decode-wspr.sh](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_wspr_122_88/app/decode-wspr.sh) script launches `write-c2-files`, `wsprd` and `curl` one after another. This script is run every two minutes by the following cron entry in [wspr.cron](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_wspr_122_88/app/wspr.cron):
{% highlight bash %}
1-59/2 * * * * cd /dev/shm && /media/mmcblk0p1/apps/sdr_transceiver_wspr_122_88/decode-wspr.sh >> decode-wspr.log 2>&1 &
{% endhighlight %}

The [transmit-wspr-message.c](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_wspr_122_88/app/transmit-wspr-message.c) program transmits WSPR messages.

GPS interface
-----

A GPS module can be used for the time synchronization and for the automatic measurement and correction of the frequency deviation.

The PPS signal should be connected to the pin DIO3_N of the [extension connector E1](http://redpitaya.readthedocs.io/en/latest/developerGuide/125-14/extent.html#extension-connector-e1). The UART interface should be connected to the UART pins of the [extension connector E2](http://redpitaya.readthedocs.io/en/latest/developerGuide/125-14/extent.html#extension-connector-e2).

The measurement and correction of the frequency deviation is disabled by default and should be enabled by uncommenting the following line in [wspr.cron](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_wspr_122_88/app/wspr.cron):
{% highlight bash %}
1-59/2 * * * * cd /dev/shm && /media/mmcblk0p1/apps/sdr_transceiver_wspr_122_88/update-corr.sh >> update-corr.log 2>&1 &
{% endhighlight %}

Getting started
-----

 - Download [SD card image zip file]({{ site.release-image }}) (more details about the SD card image can be found at [this link]({{ "/alpine/" | prepend: site.baseurl }})).
 - Copy the contents of the SD card image zip file to a micro SD card.
 - Optionally, to start the application automatically at boot time, copy its `start.sh` file from `apps/sdr_transceiver_wspr_122_88` to the topmost directory on the SD card.
 - Install the micro SD card in the STEMlab SDR board and connect the power.

Configuring WSPR receiver
-----

By default, the uploads to [wsprnet.org](http://wsprnet.org) are disabled and all the decoded data are accumulated in `/dev/shm/ALL_WSPR.TXT`.

All the configuration files and scripts can be found in the `apps/sdr_transceiver_wspr_122_88` directory on the SD card.

To enable uploads, the `CALL` and `GRID` variables should be specified in [decode-wspr.sh](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_wspr_122_88/app/decode-wspr.sh#L4-L5). These variables should be set to the call sign of the receiving station and its 6-character Maidenhead grid locator.

The frequency correction ppm value can be adjusted by editing the corr parameter in [write-c2-files.cfg](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_wspr_122_88/app/write-c2-files.cfg).

The bands list in [write-c2-files.cfg](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_wspr_122_88/app/write-c2-files.cfg) contains all the WSPR frequencies. They can be enabled or disabled by uncommenting or by commenting the corresponding lines.

Configuring WSPR transmitter
-----

The WSPR message, transmit frequency and frequency ppm value can be adjusted by editing [transmit-wspr-message.cfg](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_wspr_122_88/app/transmit-wspr-message.cfg).


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

Building `sdr_transceiver_wspr_122_88.bit`:
{% highlight bash %}
make NAME=sdr_transceiver_wspr_122_88 PART=xc7z020clg400-1 bit
{% endhighlight %}

Building SD card image zip file:
{% highlight bash %}
source helpers/build-all.sh
{% endhighlight %}
