---
title: Pulsed NMR system
---

This is a work in progress...

## Interesting links

Some interesting links on pulsed nuclear magnetic resonance:

 - [Pulsed NMR at UW](https://courses.washington.edu/phys431/PNMR/pulsed_nmr.php)

 - [Pulsed NMR at MSU](https://web.pa.msu.edu/courses/2016spring/PHY451/Experiments/pulsed_nmr.html)

 - [The Basics of NMR](https://www.cis.rit.edu/htbooks/nmr) by Joseph P. Hornak

## Short description

The system consists of one in-phase/quadrature (I/Q) digital down-converter (DDC) and of one pulse generator.

The tunable frequency range covers from 0 Hz to 60 MHz.

The I/Q data rate is configurable and six settings are available: 25, 50, 125, 250, 500, 1250 kSPS.

## Hardware

The basic blocks of the system are shown in the following diagram:

![Pulsed NMR](/img/pulsed-nmr.png)

The [projects/pulsed_nmr](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/pulsed_nmr) directory contains three Tcl files: [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/pulsed_nmr/block_design.tcl), [rx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/pulsed_nmr/rx.tcl), [tx.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/pulsed_nmr/tx.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

## Software

The [projects/pulsed_nmr/server](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/pulsed_nmr/server) directory contains the source code of the TCP server ([pulsed-nmr.c](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/pulsed_nmr/server/pulsed-nmr.c)) that receives control commands and transmits the I/Q data streams (up to 4 x 32 bit x 1250 kSPS = 152 Mbit/s) to the control program running on a remote PC.

The [projects/pulsed_nmr/client](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/pulsed_nmr/client) directory contains the source code of the control program ([pulsed_nmr.py](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/pulsed_nmr/client/pulsed_nmr.py)).

![Pulsed NMR client](/img/pulsed-nmr-client.png)

## Getting started with GNU/Linux

 - Download [SD card image zip file]({{ site.release_image }}) (more details about the SD card image can be found at [this link](/alpine.md)).
 - Copy the contents of the SD card image zip file to a micro SD card.
 - Optionally, to start the application automatically at boot time, copy its `start.sh` file from `apps/pulsed_nmr` to the topmost directory on the SD card.
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
cd red-pitaya-notes/projects/pulsed_nmr/client
python3 pulsed_nmr.py
```

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

Building `pulsed_nmr.bit`:
```bash
make NAME=pulsed_nmr bit
```

Building SD card image zip file:
```bash
source helpers/build-all.sh
```
