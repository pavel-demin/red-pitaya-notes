---
layout: page
title: LED blinker
permalink: /led-blinker/
---
For my experiments with the [Red Pitaya](http://wiki.redpitaya.com), I'd like to have the following development environment:

 - recent version of the [Vivado Design Suite](http://www.xilinx.com/products/design-tools/vivado)
 - recent version of the [Linux kernel from Xilinx](http://github.com/Xilinx/linux-xlnx/tree/xilinx-v2014.3)
 - recent version of the [Ubuntu distribution](http://wiki.ubuntu.com/TrustyTahr/ReleaseNotes) on the development machine
 - recent version of the [Debian distribution](http://www.debian.org/releases/stable) on the Red Pitaya
 - basic project with all the [Red Pitaya](http://wiki.redpitaya.com) peripherals connected
 - mostly command-line tools
 - shallow directory structure

Here is how I set it all up.

Pre-requirements
-----

I'm skipping the installation of the development machine for the time being.

My development machine has the following installed:

 - [Ubuntu](http://wiki.ubuntu.com/TrustyTahr/ReleaseNotes) 14.04.1 (amd64)

 - [Vivado Design Suite](http://www.xilinx.com/products/design-tools/vivado) 2014.3.1 with full SDK

Here is the command to install all the other required packages:
{% highlight bash %}
sudo apt-get --no-install-recommends install \
  build-essential git curl bc u-boot-tools \
  libxrender1 libxtst6 libxi6 lib32ncurses5 \
  gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf \
  qemu-user-static debootstrap binfmt-support
{% endhighlight %}

Source code
-----

The source code is available at

<https://github.com/pavel-demin/red-pitaya-notes>

This repository contains the following components:

 - `Makefile` that builds everything (almost)
 - `cfg` directory with constraints and board definition files
 - `led_blinker` directory with two Verilog files for a basic project
 - `scripts` directory with
   - Tcl scripts for Vivado and SDK
   - shell scripts that bootstrap a Debian root file system and build a bootable SD card

Getting started
-----

Setting up the Vivado environment:
{% highlight bash %}
source /opt/Xilinx/Vivado/2014.3.1/settings64.sh
source /opt/Xilinx/SDK/2014.3.1/settings64.sh
{% endhighlight %}

Cloning the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
{% endhighlight %}

Building `u-boot.elf`, `boot.bin` and `devicetree.dtb`:
{% highlight bash %}
make NAME=led_blinker all
{% endhighlight %}

Building a Debian Wheezy root file system:
{% highlight bash %}
sudo source scripts/rootfs.sh
{% endhighlight %}

Building a bootable SD card:
{% highlight bash %}
sudo source scripts/sdcard.sh
{% endhighlight %}

Reprogramming FPGA
-----

It's possible to reprogram the FPGA by loading the bitstream file into `/dev/xdevcfg`:
{% highlight bash %}
cat led_blinker.bit > /dev/xdevcfg
{% endhighlight %}
