---
layout: page
title: LED blinker
---

Introduction
-----

For my experiments with the [Red Pitaya](https://redpitaya.readthedocs.io), I'd like to have the following development environment:

 - recent version of the [Vitis Core Development Kit](https://www.xilinx.com/products/design-tools/vitis.html)
 - recent version of the [Linux kernel](https://www.kernel.org)
 - recent version of the [Debian distribution](https://www.debian.org/releases/bookworm) on the development machine
 - recent version of the [Debian distribution](https://www.debian.org/releases/bookworm) on the the Red Pitaya board
 - basic project with all the [Red Pitaya](https://redpitaya.readthedocs.io) peripherals connected
 - mostly command-line tools
 - shallow directory structure

Here is how I set it all up.

Pre-requirements
-----

My development machine has the following installed:

 - [Debian](https://www.debian.org/releases/bookworm) 12 (amd64)

 - [Vitis Core Development Kit](https://www.xilinx.com/products/design-tools/vitis.html) 2023.1

Here are the commands to install all the other required packages:
{% highlight bash %}
apt-get update

apt-get --no-install-recommends install \
  bc binfmt-support bison build-essential ca-certificates curl \
  debootstrap device-tree-compiler dosfstools flex fontconfig git \
  libgtk-3-0 libncurses-dev libssl-dev libtinfo5 parted qemu-user-static \
  squashfs-tools sudo u-boot-tools x11-utils xvfb zerofree zip
{% endhighlight %}

Source code
-----

The source code is available at

<https://github.com/pavel-demin/red-pitaya-notes>

This repository contains the following components:

 - [Makefile](https://github.com/pavel-demin/red-pitaya-notes/blob/master/Makefile) that builds everything (almost)
 - [cfg](https://github.com/pavel-demin/red-pitaya-notes/tree/master/cfg) directory with constraints and board definition files
 - [cores](https://github.com/pavel-demin/red-pitaya-notes/tree/master/cores) directory with IP cores written in Verilog
 - [projects](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects) directory with Vivado projects written in Tcl
 - [scripts](https://github.com/pavel-demin/red-pitaya-notes/tree/master/scripts) directory with
   - Tcl scripts for Vivado and SDK
   - shell scripts that build a bootable SD card and SD card image

All steps of the development chain and the corresponding scripts are shown in the following diagram:

![Scripts]({% link img/scripts.png %})

Syntactic sugar for IP cores
-----

The [projects/led_blinker](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/led_blinker) directory contains one Tcl file [block_design.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/projects/led_blinker/block_design.tcl) that instantiates, configures and interconnects all the needed IP cores.

By default, the IP core instantiation and configuration commands are quite verbose:
{% highlight Tcl %}
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ps_0

set_property CONFIG.PCW_IMPORT_BOARD_PRESET cfg/red_pitaya.xml [get_bd_cells ps_0]

connect_bd_net [get_bd_pins ps_0/FCLK_CLK0] [get_bd_pins ps_0/M_AXI_GP0_ACLK]
{% endhighlight %}

With the Tcl's flexibility, it's easy to define a less verbose command that looks similar to the module instantiation in Verilog:
{% highlight Tcl %}
cell xilinx.com:ip:processing_system7:5.5 ps_0 {
  PCW_IMPORT_BOARD_PRESET cfg/red_pitaya.xml
} {
  M_AXI_GP0_ACLK ps_0/FCLK_CLK0
}
{% endhighlight %}

The `cell` command and other helper commands are defined in the [scripts/project.tcl](https://github.com/pavel-demin/red-pitaya-notes/blob/master/scripts/project.tcl) script.

Getting started
-----

Setting up the Vitis and Vivado environment:
{% highlight bash %}
source /opt/Xilinx/Vitis/2023.1/settings64.sh
{% endhighlight %}

Cloning the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
{% endhighlight %}

Building Vivado project:
{% highlight bash %}
make NAME=led_blinker xpr
{% endhighlight %}

Opening Vivado project:
{% highlight bash %}
vivado tmp/led_blinker.xpr
{% endhighlight %}

Building bitstream file:
{% highlight bash %}
make NAME=led_blinker bit
{% endhighlight %}

SD card image
-----

Building `boot.bin`:
{% highlight bash %}
make NAME=led_blinker all
{% endhighlight %}

Building a bootable SD card image:
{% highlight bash %}
sudo sh scripts/image.sh scripts/debian.sh red-pitaya-debian-12-armhf.img 1024
{% endhighlight %}

The SD card image size is 1 GB, so it should fit on any SD card starting from 2 GB.

To write the image to a SD card, the `dd` command-line utility can be used on GNU/Linux and Mac OS X or [Win32 Disk Imager](https://sourceforge.net/projects/win32diskimager) can be used on MS Windows.

The default password for the `root` account is `changeme`.

A pre-built SD card image can be downloaded from [this link](https://www.dropbox.com/scl/fi/y9fyu3xe2nrxcb4qcrslz/red-pitaya-debian-12.4-armhf-20240204.zip?rlkey=e90izxqcoj821vslt5fo82wel&dl=1).

Resizing SD card partitions on running Red Pitaya:
{% highlight bash %}
# delete second partition
echo -e "d\n2\nw" | fdisk /dev/mmcblk0
# recreate partition
parted -s /dev/mmcblk0 mkpart primary ext4 16MiB 100%
# resize partition
resize2fs /dev/mmcblk0p2
{% endhighlight %}

Reprogramming FPGA
-----

It's possible to reprogram the FPGA by loading the bitstream file into `/dev/xdevcfg`:
{% highlight bash %}
cat led_blinker.bit > /dev/xdevcfg
{% endhighlight %}
