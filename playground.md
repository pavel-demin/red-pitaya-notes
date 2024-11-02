---
title: Playground
---

## Introduction

The combination of Jupyter notebooks, the [pyhubio](https://github.com/pavel-demin/pyhubio) library and the [AXI4 hub](/axi-hub.md) allows interactive communication with all parts of the FPGA configuration and visualization of input and output data, making testing and prototyping more dynamic.

The [notebooks](https://github.com/pavel-demin/red-pitaya-notes/tree/master/notebooks) directory contains a few examples of Jupyter notebooks.

![Jupyter notebooks](/img/jupyter-notebooks.png)

## Hardware

The basic blocks of the playground project are shown in the following diagram:

![Playground](/img/playground.png)

The [projects/playground](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/playground) directory contains one Tcl file [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/playground/block_design.tcl) that instantiates, configures and interconnects all the needed IP cores.

A pre-built Vivado project can be found in the `playground` directory in the [release zip file]({{ site.release_file }}).

## Getting started

- Download [SD card image zip file]({{ site.release_image }}) (more details about the SD card image can be found at [this link](/alpine.md))
- Copy the contents of the SD card image zip file to a micro SD card
- Optionally, to start the application automatically at boot time, copy its `start.sh` file from `apps/playground` to the topmost directory on the SD card
- Install the micro SD card in the Red Pitaya board and connect the power
- Download and unpack the [release zip file]({{ site.release_file }})

- Install Visual Studio Code following the platform-specific instructions below:
  - [macOS](https://code.visualstudio.com/docs/setup/mac)
  - [Linux](https://code.visualstudio.com/docs/setup/linux)
  - [Windows](https://code.visualstudio.com/docs/setup/windows)

- Install the following Visual Studio Code extensions:
  - [Python](https://marketplace.visualstudio.com/items?itemName=ms-python.python)
  - [Jupyter](https://marketplace.visualstudio.com/items?itemName=ms-toolsai.jupyter)
  - [Micromamba](https://marketplace.visualstudio.com/items?itemName=corker.vscode-micromamba)

- Open [notebooks](https://github.com/pavel-demin/red-pitaya-notes/tree/master/notebooks) directory in Visual Studio Code:
  - From the "File" menu select "Open Folder"
  - In the "Open Folder" dialog find and select [notebooks](https://github.com/pavel-demin/red-pitaya-notes/tree/master/notebooks) directory and click "Open"

- Create micromamba environment:
  - From the "View" menu select "Command Palette"
  - Type "micromamba create environment"

## Working with notebooks

- Open one of the notebooks in Visual Studio Code

- Make sure the micromamba environment called "red-pitaya-notes" is selected in the kernel/environment selector in the top right corner of the notebook view

- Run the code cells one by one by clicking the play icon in the top left corner of each cell
