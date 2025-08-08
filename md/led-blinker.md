# LED blinker

## Introduction

For my experiments with the [Red Pitaya](https://redpitaya.readthedocs.io), I'd like to have the following development environment:

- recent version of the [Vitis Core Development Kit](https://www.amd.com/en/products/software/adaptive-socs-and-fpgas/vitis.html)
- recent version of the [Linux kernel](https://www.kernel.org)
- recent version of the [Debian distribution](https://www.debian.org/releases/trixie) on the development machine
- recent version of the [Debian distribution](https://www.debian.org/releases/trixie) on the the Red Pitaya board
- basic project with all the [Red Pitaya](https://redpitaya.readthedocs.io) peripherals connected
- mostly command-line tools
- shallow directory structure

Here is how I set it all up.

## Pre-requirements

My development machine has the following installed:

- [Debian](https://www.debian.org/releases/trixie) 13 (amd64)

- [Vitis Core Development Kit](https://www.amd.com/en/products/software/adaptive-socs-and-fpgas/vitis.html) 2025.1

Here are the commands to install all the other required packages:

```bash
apt-get update

apt-get --no-install-recommends install \
  bc binfmt-support bison build-essential ca-certificates curl \
  debootstrap device-tree-compiler dosfstools flex fontconfig git \
  libgtk-3-0 libncurses-dev libssl-dev libtinfo5 parted qemu-user-static \
  squashfs-tools sudo u-boot-tools x11-utils xvfb zerofree zip
```

## Source code

The source code is available at

<https://github.com/pavel-demin/red-pitaya-notes>

This repository contains the following components:

- [Makefile]($source$/Makefile) that builds everything (almost)
- [cfg]($source$/cfg) directory with constraints and board definition files
- [cores]($source$/cores) directory with IP cores written in Verilog
- [projects]($source$/projects) directory with Vivado projects written in Tcl
- [scripts]($source$/scripts) directory with
  - Tcl scripts for Vivado and SDK
  - shell scripts that build a bootable SD card and SD card image

All steps of the development chain and the corresponding scripts are shown in the following diagram:

![Scripts](/img/scripts.png)

## Syntactic sugar for IP cores

The [projects/led_blinker]($source$/projects/led_blinker) directory contains one Tcl file [block_design.tcl]($source$/projects/led_blinker/block_design.tcl) that instantiates, configures and interconnects all the needed IP cores.

By default, the IP core instantiation and configuration commands are quite verbose:

```Tcl
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ps_0

set_property CONFIG.PCW_IMPORT_BOARD_PRESET cfg/red_pitaya.xml [get_bd_cells ps_0]

connect_bd_net [get_bd_pins ps_0/FCLK_CLK0] [get_bd_pins ps_0/M_AXI_GP0_ACLK]
```

With the Tcl's flexibility, it's easy to define a less verbose command that looks similar to the module instantiation in Verilog:

```Tcl
cell xilinx.com:ip:processing_system7:5.5 ps_0 {
  PCW_IMPORT_BOARD_PRESET cfg/red_pitaya.xml
} {
  M_AXI_GP0_ACLK ps_0/FCLK_CLK0
}
```

The `cell` command and other helper commands are defined in the [scripts/project.tcl]($source$/scripts/project.tcl) script.

## Getting started

Setting up the Vitis and Vivado environment:

```bash
source /opt/Xilinx/2025.1/Vitis/settings64.sh
```

Cloning the source code repository:

```bash
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
```

Building Vivado project:

```bash
make NAME=led_blinker xpr
```

Opening Vivado project:

```bash
vivado tmp/led_blinker.xpr
```

Building bitstream file:

```bash
make NAME=led_blinker bit
```

## SD card image

Building `boot.bin`:

```bash
make NAME=led_blinker all
```

Building a bootable SD card image:

```bash
sudo sh scripts/image.sh scripts/debian.sh red-pitaya-debian-12-armhf.img 1024
```

The SD card image size is 1 GB, so it should fit on any SD card starting from 2 GB.

To write the image to a SD card, the `dd` command-line utility can be used on GNU/Linux and Mac OS X or [Win32 Disk Imager](https://sourceforge.net/projects/win32diskimager) can be used on MS Windows.

The default password for the `root` account is `changeme`.

A pre-built SD card image can be downloaded from [this link](https://www.dropbox.com/scl/fi/yu38uxxeagjpx1ww0bchs/red-pitaya-debian-12.8-armhf-20241222.zip?rlkey=3aacc87x8sucdw07raufx3vim&dl=1).

Resizing SD card partitions on running Red Pitaya:

```bash
# delete second partition
echo -e "d\n2\nw" | fdisk /dev/mmcblk0
# recreate partition
parted -s /dev/mmcblk0 mkpart primary ext4 16MiB 100%
# resize partition
resize2fs /dev/mmcblk0p2
```

## Reprogramming FPGA

It's possible to reprogram the FPGA by loading the bitstream file into `/dev/xdevcfg`:

```bash
cat led_blinker.bit > /dev/xdevcfg
```
