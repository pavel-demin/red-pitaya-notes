# Pulsed NMR system

This is a work in progress...

## Interesting links

Some interesting links on pulsed nuclear magnetic resonance:

- [Pulsed NMR at UW](https://courses.washington.edu/phys431/PNMR/pulsed_nmr.php)

- [Pulsed NMR at MSU](https://web.pa.msu.edu/courses/2016spring/PHY451/Experiments/pulsed_nmr.html)

- [The Basics of NMR](https://www.cis.rit.edu/htbooks/nmr) by Joseph P. Hornak

## Short description

The system consists of one in-phase/quadrature (I/Q) digital down-converter (DDC) and of one pulse generator.

The tunable frequency range covers from 0 Hz to 60 MHz.

The I/Q data rate is configurable and seven settings are available: 20, 40, 80, 160, 320, 640, 1280 kSPS.

## Hardware

The basic blocks of the system are shown in the following diagram:

![Pulsed NMR](/img/pulsed-nmr-122-88.png)

The [projects/pulsed_nmr_122_88]($source$/projects/pulsed_nmr_122_88) directory contains three Tcl files: [block_design.tcl]($source$/projects/pulsed_nmr_122_88/block_design.tcl), [rx.tcl]($source$/projects/pulsed_nmr_122_88/rx.tcl), [tx.tcl]($source$/projects/pulsed_nmr_122_88/tx.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

The source code of the [R](https://www.r-project.org) script used to calculate the coefficients of the FIR filter can be found in [projects/pulsed_nmr_122_88/filters/fir_0.r]($source$/projects/pulsed_nmr_122_88/filters/fir_0.r).

## RF and GPIO connections

- two digital down-converters (DDC) are connected to IN1 and IN2
- pulse generator is connected to OUT1
- continuous cosine signal of the receiver's DDS is connected to OUT2
- digital output for RX/TX switch control is connected to pin DIO0_N of the [extension connector E1](https://redpitaya.readthedocs.io/en/latest/developerGuide/hardware/ORIG_GEN/122-16/top.html#extension-connector-e1)
- general purpose digital outputs are connected to the pins DIO0_P - DIO7_P of the [extension connector E1](https://redpitaya.readthedocs.io/en/latest/developerGuide/hardware/ORIG_GEN/122-16/top.html#extension-connector-e1)

## Software

The [projects/pulsed_nmr_122_88/server]($source$/projects/pulsed_nmr_122_88/server) directory contains the source code of the TCP server ([pulsed-nmr.c]($source$/projects/pulsed_nmr_122_88/server/pulsed-nmr.c)) that receives control commands and transmits the I/Q data streams (up to 4 x 32 bit x 1280 kSPS = 156 Mbit/s) to the control program running on a remote PC.

The [projects/pulsed_nmr_122_88/client]($source$/projects/pulsed_nmr_122_88/client) directory contains the source code of the control program ([pulsed_nmr.py]($source$/projects/pulsed_nmr_122_88/client/pulsed_nmr.py)).

![Pulsed NMR client](/img/pulsed-nmr-client.png)

## .NET library

The .NET library provides a minimalist set of commands for controlling all parameters of the signal processing modules and programming the pulse generator. The commands can be classified into five groups:

Commands to manage the connection with the TCP server:

- Connect(address)
- Disconnect()

Commands to set the frequencies and decimation rate:

- SetFreqRX(frequency)
- SetFreqTX(frequency)
- SetRateRX(rate)

Commands to program a pulse sequence:

- ClearEvents()
- AddEventTX(delay, sync, gate, level, tx_phase, rx_phase)
- AddEventRX(size, read)

Command to start pulse sequence and receive data:

- RecieveData(size)

Various commands to control output RF signal level, GPIO pins and an external DAC:

- SetLevelTX(level)
- SetPin(pin)
- ClearPin(pin)
- SetDAC(data)

The source code of the .NET library can be found in [projects/pulsed_nmr/client/PulsedNMR.cs]($source$/projects/pulsed_nmr/client/PulsedNMR.cs).

## Getting started with GNU/Linux

- Download [SD card image zip file]($release_image$) (more details about the SD card image can be found at [this link](/alpine/)).
- Copy the contents of the SD card image zip file to a micro SD card.
- Optionally, to start the application automatically at boot time, copy its `start.sh` file from `apps/pulsed_nmr_122_88` to the topmost directory on the SD card.
- Install required Python libraries:

```bash
sudo apt-get install python3-numpy python3-matplotlib python3-pyqt6
```

- Clone the source code repository:

```bash
git clone https://github.com/pavel-demin/red-pitaya-notes
```

- Run the control program:

```bash
cd red-pitaya-notes/projects/pulsed_nmr_122_88/client
python3 pulsed_nmr.py
```

## Building from source

The installation of the development machine is described at [this link](/development-machine/).

The structure of the source code and of the development chain is described at [this link](/led-blinker/).

Setting up the Vitis and Vivado environment:

```bash
source /opt/Xilinx/2025.1/Vitis/settings64.sh
```

Cloning the source code repository:

```bash
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
```

Building `pulsed_nmr_122_88.bit`:

```bash
make NAME=pulsed_nmr_122_88 PART=xc7z020clg400-1 bit
```

Building SD card image zip file:

```bash
source helpers/build-all.sh
```
