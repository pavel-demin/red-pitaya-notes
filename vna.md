---
title: Vector Network Analyzer
---

## Interesting links

Some interesting links on network analyzers:

 - [VNA Operating Guide](https://www.dropbox.com/sh/5fy49wae6xwxa8a/AADK4ci3Bv4YeqlIAsMBWErNa/vna/VNA_Guide.pdf?dl=1)

 - [Three-Bead-Balun Reflection Bridge](https://www.dropbox.com/sh/5fy49wae6xwxa8a/AAAFReIzG5tnpxZKTaOrhn4wa/vna/3BeadBalunBridge.pdf?dl=1)

 - [Ham VNA](https://dxatlas.com/HamVNA)

 - [Introduction to Network Analyzer Measurements](https://download.ni.com/evaluation/rf/Introduction_to_Network_Analyzer_Measurements.pdf)

## Hardware

The basic blocks of the system are shown in the following diagram:

![Vector Network Analyzer](/img/vna.png)

The [projects/vna](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/vna) directory contains one Tcl file [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/vna/block_design.tcl) that instantiates, configures and interconnects all the needed IP cores.

## Software

The [projects/vna/server](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/vna/server) directory contains the source code of the TCP server ([vna.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/vna/server/vna.c)) that receives control commands and transmits the data to the control program running on a remote PC.

The [projects/vna/client](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/vna/client) directory contains the source code of the control program ([vna.py](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/vna/client/vna.py)).

![VNA client](/img/vna-client.png)

## Getting started with MS Windows

 - Download [SD card image zip file]({{ site.release_image }}) (more details about the SD card image can be found at [this link](/alpine.md)).
 - Copy the contents of the SD card image zip file to a micro SD card.
 - Optionally, to start the application automatically at boot time, copy its `start.sh` file from `apps/vna` to the topmost directory on the SD card.
 - Install the micro SD card in the Red Pitaya board and connect the power.
 - Download and unpack the [release zip file]({{ site.release_file }}).
 - Run the `vna.exe` program in the `control` directory.
 - Type in the IP address of the Red Pitaya board and press Connect button.
 - Perform calibration and measurements.

## Getting started with GNU/Linux

 - Download [SD card image zip file]({{ site.release_image }}) (more details about the SD card image can be found at [this link](/alpine.md)).
 - Copy the contents of the SD card image zip file to a micro SD card.
 - Optionally, to start the application automatically at boot time, copy its `start.sh` file from `apps/vna` to the topmost directory on the SD card.
 - Install the micro SD card in the Red Pitaya board and connect the power.
 - Install required Python libraries:
```bash
sudo apt-get install python3-numpy python3-matplotlib python3-pyqt5
```
 - Clone the source code repository:
```bash
git clone https://github.com/pavel-demin/red-pitaya-notes
```
 - Run the control program:
```bash
cd red-pitaya-notes/projects/vna/client
python3 vna.py
```
 - Type in the IP address of the Red Pitaya board and press Connect button.
 - Perform calibration and measurements.

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

Building `vna.bit`:
```bash
make NAME=vna bit
```

Building SD card image zip file:
```bash
source helpers/build-all.sh
```
