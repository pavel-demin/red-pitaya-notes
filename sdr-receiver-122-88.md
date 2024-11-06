---
title: SDR receiver
---

## Hardware

The FPGA configuration consists of sixteen identical digital down-converters (DDC). Their structure is shown in the following diagram:

![SDR receiver](/img/sdr-transceiver-hpsdr-ddc-122-88.png)

The I/Q data rate is configurable and four settings are available: 48, 96, 192, 384 kSPS.

The tunable frequency range covers from 0 Hz to 490 MHz.

The [projects/sdr_receiver_122_88](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver_122_88) directory contains two Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_receiver_122_88/block_design.tcl), [rx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_receiver_122_88/rx.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

The [projects/sdr_receiver_122_88/filters](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver_122_88/filters) directory contains the source code of the [R](https://www.r-project.org) scripts used to calculate the coefficients of the FIR filters.

## Software

The [projects/sdr_receiver_122_88/server](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver_122_88/server) directory contains the source code of the TCP server ([sdr-receiver.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/sdr_receiver_122_88/server/sdr-receiver.c)) that receives control commands and transmits the I/Q data streams to the SDR programs.

The [SDR SMEM](https://github.com/pavel-demin/sdr-smem) repository contains the source code of the TCP client ([tcp_smem.lpr](https://github.com/pavel-demin/sdr-smem/blob/main/tcp_smem.lpr)), ExtIO plug-in ([extio_smem.lpr](https://github.com/pavel-demin/sdr-smem/blob/main/extio_smem.lpr)) and other programs and plug-ins. The following diagram shows an example of how these programs and plug-ins can be used:

![SDR SMEM](/img/sdr-smem.png)

The `tcp_smem` program runs on a computer. It receives the I/Q data streams over the network and transfers them to other programs and plugins via shared memory.

## Antenna

I use simple indoor antennas made from a single loop of non-coaxial wire. Their approximate scheme is shown in the following diagrams:

![Antenna](/img/antenna.png)

## Getting started

- Connect an antenna to the IN1 connector on the Red Pitaya board.
- Download [SD card image zip file]({{ site.release_image }}) (more details about the SD card image can be found at [this link](/alpine.md)).
- Copy the contents of the SD card image zip file to a micro SD card.
- Optionally, to start the application automatically at boot time, copy its `start.sh` file from `apps/sdr_receiver_122_88` to the topmost directory on the SD card.
- Install the micro SD card in the Red Pitaya board and connect the power.
- Download and install [SDR#](https://www.dropbox.com/sh/5fy49wae6xwxa8a/AAAdAcU238cppWziK4xPRIADa/sdr/sdrsharp_v1.0.0.1361_with_plugins.zip?dl=1) or [HDSDR](https://www.hdsdr.de).
- Download and unpack the [SDR SMEM zip file]({{ site.sdr_smem_file }}).
- Copy `extio_smem.dll` into the SDR# or HDSDR installation directory.
- Start SDR# or HDSDR.
- Select SMEM from the Source list in SDR# or from the Options [F7] &rarr; Select Input menu in HDSDR.
- Press Configure icon in SDR# or press SDR-Device [F8] button in HDSDR, then enter the IP address of the Red Pitaya board.
- Press Play icon in SDR# or press Start [F2] button in HDSDR.

## Running CW Skimmer Server and Reverse Beacon Network Aggregator

- Install [CW Skimmer Server](https://dxatlas.com/skimserver).
- Download and unpack the [SDR SMEM zip file]({{ site.sdr_smem_file }}).
- Make a copy of the `tcp_smem.exe` program and rename the copy to `tcp_smem_1.exe`.
- Start `tcp_smem.exe` and `tcp_smem_1.exe`, enter the IP addresses of the Red Pitaya board and press the Connect button.
- Copy `intf_smem.dll` to the CW Skimmer Server program directory (`C:\Program Files (x86)\Afreet\SkimSrv`).
- Make a copy of the `SkimSrv` directory and rename the copy to `SkimSrv2`.
- In the `SkimSrv2` directory, rename `SkimSrv.exe` to `SkimSrv2.exe` and rename `intf_smem.dll` to `intf_smem_1.dll`.
- Install [Reverse Beacon Network Aggregator](https://www.reversebeacon.net/pages/Aggregator+34).
- Start `SkimSrv.exe` and `SkimSrv2.exe`, configure frequencies and your call sign.
- Start Reverse Beacon Network Aggregator.

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

Building `sdr_receiver.bit`:

```bash
make NAME=sdr_receiver_122_88 PART=xc7z020clg400-1 bit
```

Building SD card image zip file:

```bash
source helpers/build-all.sh
```
