---
title: Multiband FT8 transceiver
---

## Short description

This project implements a standalone multiband FT8 transceiver with all the FT8 signal processing done by STEMlab SDR in the following way:

- simultaneously record FT8 signals from sixteen bands
- use FPGA for all the conversions needed to produce .c2 files (complex 32-bit floating-point data at 4000 samples per second)
- use on-board CPU to process the .c2 files with the [FT8 decoder](https://github.com/pavel-demin/ft8d)

With this configuration, it is enough to connect STEMlab SDR to an antenna and to a network. After switching STEMlab SDR on, it will automatically start operating as a FT8 receiver.

## Hardware

The FPGA configuration consists of sixteen identical digital down-converters (DDC). Their structure is shown in the following diagram:

![FT8 receiver](/img/sdr-receiver-ft8-122-88.png)

The DDC output contains complex 32-bit floating-point data at 4000 samples per second.

The [projects/sdr_transceiver_ft8_122_88](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_ft8_122_88) directory contains three Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_ft8_122_88/block_design.tcl), [rx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_ft8_122_88/rx.tcl) and [tx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_transceiver_ft8_122_88/tx.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

## Software

The [write-c2-files.c](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_ft8_122_88/app/write-c2-files.c) program accumulates 236000 samples at 4000 samples per second for each of the sixteen bands and saves the samples to sixteen .c2 files.

The recorded .c2 files are processed with the [FT8 decoder](https://github.com/pavel-demin/ft8d).

The [decode-ft8.sh](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_ft8_122_88/app/decode-ft8.sh) script launches `write-c2-files` and `ft8d` one after another. This script is run every minute by the following cron entry in [ft8.cron](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_ft8_122_88/app/ft8.cron):

```bash
* * * * * cd /dev/shm && /media/mmcblk0p1/apps/sdr_transceiver_ft8_122_88/decode-ft8.sh >> decode-ft8.log 2>&1 &
```

## GPS interface

A GPS module can be used for the time synchronization and for the automatic measurement and correction of the frequency deviation.

The PPS signal should be connected to the pin DIO3_N of the [extension connector E1](https://redpitaya.readthedocs.io/en/latest/developerGuide/hardware/125-14/extent.html#extension-connector-e1). The UART interface should be connected to the UART pins of the [extension connector E2](https://redpitaya.readthedocs.io/en/latest/developerGuide/hardware/125-14/extent.html#extension-connector-e2).

The measurement and correction of the frequency deviation is disabled by default and should be enabled by uncommenting the following line in [ft8.cron](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_ft8_122_88/app/ft8.cron):

```bash
* * * * * cd /dev/shm && /media/mmcblk0p1/apps/common_tools/update-corr.sh 122.88 >> update-corr.log 2>&1 &
```

## Getting started

- Download [SD card image zip file]({{ site.release_image }}) (more details about the SD card image can be found at [this link](/alpine.md)).
- Copy the contents of the SD card image zip file to a micro SD card.
- Optionally, to start the application automatically at boot time, copy its `start.sh` file from `apps/sdr_transceiver_ft8_122_88` to the topmost directory on the SD card.
- Install the micro SD card in the STEMlab SDR board and connect the power.

## Configuring FT8 receiver

All the configuration files and scripts can be found in the `apps/sdr_transceiver_ft8_122_88` directory on the SD card.

To enable uploads, the `CALL` and `GRID` variables should be specified in [upload-ft8.sh](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_ft8_122_88/app/upload-ft8.sh#L4-L5). These variables should be set to the call sign of the receiving station and its 6-character Maidenhead grid locator.

The frequency correction ppm value can be adjusted by editing the corr parameter in [write-c2-files.cfg](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_ft8_122_88/app/write-c2-files.cfg).

The bands list in [write-c2-files.cfg](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_transceiver_ft8_122_88/app/write-c2-files.cfg) contains all the FT8 frequencies. They can be enabled or disabled by uncommenting or by commenting the corresponding lines.

## Building from source

The installation of the development machine is described at [this link](/development-machine.md).

The structure of the source code and of the development chain is described at [this link](/led-blinker.md).

Setting up the Vitis and Vivado environment:

```bash
source /opt/Xilinx/Vitis/2023.1/settings64.sh
```

Cloning the source code repository:

```bash
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
```

Building `sdr_transceiver_ft8_122_88.bit`:

```bash
make NAME=sdr_transceiver_ft8_122_88 PART=xc7z020clg400-1 bit
```

Building SD card image zip file:

```bash
source helpers/build-all.sh
```