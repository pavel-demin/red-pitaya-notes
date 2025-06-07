# Multichannel Pulse Height Analyzer

## Interesting links

Some interesting links on radiation spectroscopy:

- [Digital signal processing for BGO detectors](<https://doi.org/10.1016/0168-9002(93)91105-V>)

- [Digital gamma-ray spectroscopy based on FPGA technology](<https://doi.org/10.1016/S0168-9002(01)01925-8>)

- [Digital signal processing for segmented HPGe detectors](https://archiv.ub.uni-heidelberg.de/volltextserver/4991)

- [FPGA-based algorithms for the stability improvement of high-flux X-ray spectrometric imaging detectors](https://tel.archives-ouvertes.fr/tel-02096235)

- [CAEN Digital Pulse Height Analyser - a digital approach to
  Radiation Spectroscopy](https://www.caen.it/documents/News/32/AN2508_Digital_Pulse_Height_Analyser.pdf)

- [Spectrum Analysis Introduction](https://www.canberra.com/literature/fundamental-principles/pdf/Spectrum-Analysis.pdf)

## Hardware

This system is designed to analyze the height of Gaussian-shaped pulses and it expects a signal from a pulse-shaping amplifier. It is also known to work with non-overlapping exponentially rising and exponentially decaying pulses.

The position of the HV/LV jumpers of the [fast analog inputs](https://redpitaya.readthedocs.io/en/latest/developerGuide/hardware/125-14/fastIO.html) should be chosen depending on the amplitude range of the input signal. Ideally, the amplitude range of the input signal should closely match the chosen hardware input range.

The basic blocks of the system are shown in the following diagrams:

![Multichannel Pulse Height Analyzer](/img/mcpha.png)

![Exponential Pulse Generator](/img/mcpha-gen.png)

The width of the pulse at the input of the pulse height analyzer module can be adjusted by varying the decimation factor of the CIC filter.

The baseline is automatically subtracted. The baseline level is defined as the minimum value just before the rising edge of the analyzed pulse.

The embedded oscilloscope can be used to check the shape of the pulse at the input of the pulse height analyzer module.

The exponential pulse generator can be used to generate signals with specified time and amplitude distributions. It consists of an impulse generator module and two IIR filters. The impulse generator module outputs a 8 ns (1 clock cycle at 125 MHz) impulse of a required amplitude and after a required time interval. The two IIR filters are used to emulate the exponentially rising and exponentially decaying edges of the generated pulses.

The [projects/mcpha]($source$/projects/mcpha) directory contains five Tcl files: [block_design.tcl]($source$/projects/mcpha/block_design.tcl), [pha.tcl]($source$/projects/mcpha/pha.tcl), [hst.tcl]($source$/projects/mcpha/hst.tcl), [osc.tcl]($source$/projects/mcpha/osc.tcl), [gen.tcl]($source$/projects/mcpha/gen.tcl). The code in these files instantiates, configures and interconnects all the needed IP cores.

The source code of the [R](https://www.r-project.org) script used to calculate the coefficients of the FIR filter can be found in [projects/mcpha/filters/fir_0.r]($source$/projects/mcpha/filters/fir_0.r).

## Software

The [projects/mcpha/server]($source$/projects/mcpha/server) directory contains the source code of the TCP server ([mcpha-server.c]($source$/projects/mcpha/server/mcpha-server.c)) that receives control commands and transmits the data to the control program running on a remote PC.

The [projects/mcpha/client]($source$/projects/mcpha/client) directory contains the source code of the control program ([mcpha.py]($source$/projects/mcpha/client/mcpha.py)).

![MCPHA client](/img/mcpha-client.png)

## Getting started with MS Windows

- Connect a signal source to the IN1 or IN2 connector on the Red Pitaya board.
- Download [SD card image zip file]($release_image$) (more details about the SD card image can be found at [this link](/alpine/)).
- Copy the contents of the SD card image zip file to a micro SD card.
- Optionally, to start the application automatically at boot time, copy its `start.sh` file from `apps/mcpha` to the topmost directory on the SD card.
- Install the micro SD card in the Red Pitaya board and connect the power.
- Download and unpack the [release zip file]($release_file$).
- Run the `mcpha.exe` program in the `control` directory.
- Type in the IP address of the Red Pitaya board and press Connect button.
- Select Spectrum histogram 1 or Spectrum histogram 2 tab.
- Adjust amplitude threshold and time of exposure.
- Press Start button.

## Getting started with GNU/Linux

- Connect a signal source to the IN1 or IN2 connector on the Red Pitaya board.
- Download [SD card image zip file]($release_image$) (more details about the SD card image can be found at [this link](/alpine/)).
- Copy the contents of the SD card image zip file to a micro SD card.
- Optionally, to start the application automatically at boot time, copy its `start.sh` file from `apps/mcpha` to the topmost directory on the SD card.
- Install the micro SD card in the Red Pitaya board and connect the power.
- Install Python 3 and all the required libraries::

```bash
sudo apt-get install python3-numpy python3-matplotlib python3-pyqt5
```

- Clone the source code repository:

```bash
git clone https://github.com/pavel-demin/red-pitaya-notes
```

- Run the control program:

```bash
cd red-pitaya-notes/projects/mcpha/client
python3 mcpha.py
```

- Type in the IP address of the Red Pitaya board and press Connect button.
- Select Spectrum histogram 1 or Spectrum histogram 2 tab.
- Adjust amplitude threshold and time of exposure.
- Press Start button.

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

Building `mcpha.bit`:

```bash
make NAME=mcpha bit
```

Building SD card image zip file:

```bash
source helpers/build-all.sh
```
