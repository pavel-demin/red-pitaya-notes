---
layout: page
title: SDR receiver without DMA
permalink: /sdr-receiver-nodma/
---

What's new
-----

To make the [SDR receiver]({{ "/sdr-receiver/" | prepend: site.baseurl }}) compatible with the Red Pitaya Ecosystem, the RAM writer block was replaced with a BRAM buffer and a couple of additional interface blocks.

![SDR receiver]({{ "/img/sdr-receiver-nodma.png" | prepend: site.baseurl }})

The [projects/sdr_receiver_nodma](https://github.com/pavel-demin/red-pitaya-notes/tree/develop/projects/sdr_receiver_nodma) directory contains one Tcl file [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/develop/projects/sdr_receiver_nodma/block_design.tcl) that instantiates, configures and interconnects all the needed IP cores.

This new version communicates with the SDR software via TCP. The port number is `1001`.

The [projects/sdr_receiver_nodma/server](https://github.com/pavel-demin/red-pitaya-notes/tree/develop/projects/sdr_receiver_nodma/server) directory contains the source code of the TCP server ([sdr-receiver.c](https://github.com/pavel-demin/red-pitaya-notes/blob/develop/projects/sdr_receiver_nodma/server/sdr-receiver.c)).

The ExtIO plug-in has now a GUI form for the IP address configuration.

The [projects/sdr_receiver_nodma/ExtIO_RedPitaya](https://github.com/pavel-demin/red-pitaya-notes/tree/develop/projects/sdr_receiver_nodma/ExtIO_RedPitaya) directory contains the source code of this plug-in.

Getting started
-----

 - Requirements:
   - Computer running MS Windows.
   - Wired Ethernet connection between the computer and the Red Pitaya board.
 - Connect an antenna to the IN2 connector on the Red Pitaya board.
 - Download [FPGA configuration file](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/SDR/sdr_receiver.bin) and the [sdr-receiver program](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/SDR/sdr-receiver).
 - Copy the downloaded files (`sdr_receiver.bin` and `sdr-receiver`) to the original Red Pitaya SD card.
 - Edit `etc/init.d/rcS` on the SD card to add the commands that configure FPGA and start the `sdr-receiver` program:
{% highlight bash %}
cat /opt/sdr_receiver.bin > /dev/xdevcfg
/opt/sdr-receiver &
{% endhighlight %}
 - Insert the SD card in Red Pitaya and connect the power.
 - Download and install [SDR#](http://sdrsharp.com/#download) or [HDSDR](http://www.hdsdr.de/).
 - Download [pre-built ExtIO plug-in](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/SDR/ExtIO_RedPitaya.dll) for SDR# and HDSDR.
 - Copy `ExtIO_RedPitaya.dll` into the SDR# or HDSDR installation directory.
 - Start SDR# or HDSDR.
 - Select Red Pitaya SDR from the Source list in SDR# or from the Options [F7] &rarr; Select Input menu in HDSDR.
 - Press Configure icon in SDR# or press ExtIO button in HDSDR, then type in the IP address of the Red Pitaya board and close the configuration window.
 - Press Play icon in SDR# or press Start [F2] button in HDSDR.
