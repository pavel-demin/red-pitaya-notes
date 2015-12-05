---
layout: page
title: LED blinker
permalink: /led-blinker/
---

Introduction
-----

For my experiments with the [Red Pitaya](http://wiki.redpitaya.com), I'd like to have the following development environment:

 - recent version of the [Vivado Design Suite](http://www.xilinx.com/products/design-tools/vivado)
 - recent version of the [Linux kernel from Xilinx](http://github.com/Xilinx/linux-xlnx/tree/xilinx-v2015.4)
 - recent version of the [Debian distribution](http://www.debian.org/releases/jessie) on the development machine
 - recent version of the [Debian distribution](http://www.debian.org/releases/jessie) on the Red Pitaya
 - basic project with all the [Red Pitaya](http://wiki.redpitaya.com) peripherals connected
 - mostly command-line tools
 - shallow directory structure

Here is how I set it all up.

Pre-requirements
-----

My development machine has the following installed:

 - [Debian](http://www.debian.org/releases/jessie) 8.2 (amd64)

 - [Vivado Design Suite](http://www.xilinx.com/products/design-tools/vivado) 2015.4 with full SDK

The installation of the development machine is described at [this link]({{ "/development-machine/" | prepend: site.baseurl }}).

Here are the commands to install all the other required packages:
{% highlight bash %}
sudo echo "deb http://emdebian.org/tools/debian jessie main" > /etc/apt/sources.list.d/emdebian.list
sudo wget -O - http://emdebian.org/tools/debian/emdebian-toolchain-archive.key | apt-key add -
sudo dpkg --add-architecture armhf
sudo apt-get update

sudo apt-get --no-install-recommends install \
  build-essential git curl ca-certificates sudo \
  libxrender1 libxtst6 libxi6 lib32ncurses5 \
  crossbuild-essential-armhf \
  bc u-boot-tools device-tree-compiler libncurses5-dev \
  libssl-dev qemu-user-static binfmt-support \
  dosfstools parted debootstrap

sudo ln -s make /usr/bin/gmake
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

The `cell` command is defined in the [scripts/project.tcl
](https://github.com/pavel-demin/red-pitaya-notes/blob/master/scripts/project.tcl) script as follows:
{% highlight Tcl %}
proc cell {cell_vlnv cell_name {cell_props {}} {cell_ports {}}} {
  set cell [create_bd_cell -type ip -vlnv $cell_vlnv $cell_name]
  set prop_list {}
  foreach {prop_name prop_value} $cell_props {
    lappend prop_list CONFIG.$prop_name $prop_value
  }
  if {[llength $prop_list] > 1} {
    set_property -dict $prop_list $cell
  }
  foreach {local_name remote_name} $cell_ports {
    set local_port [get_bd_pins $cell_name/$local_name]
    set remote_port [get_bd_pins $remote_name]
    if {[llength $local_port] == 1 && [llength $remote_port] == 1} {
      connect_bd_net $local_port $remote_port
      continue
    }
    set local_port [get_bd_intf_pins $cell_name/$local_name]
    set remote_port [get_bd_intf_pins $remote_name]
    if {[llength $local_port] == 1 && [llength $remote_port] == 1} {
      connect_bd_intf_net $local_port $remote_port
      continue
    }
    error "** ERROR: can't connect $cell_name/$local_name and $remote_port"
  }
}
{% endhighlight %}

Getting started
-----

Setting up the Vivado environment:
{% highlight bash %}
source /opt/Xilinx/Vivado/2015.4/settings64.sh
source /opt/Xilinx/SDK/2015.4/settings64.sh
{% endhighlight %}

Cloning the source code repository:
{% highlight bash %}
git clone https://github.com/pavel-demin/red-pitaya-notes
cd red-pitaya-notes
{% endhighlight %}

Building `boot.bin`, `devicetree.dtb` and `uImage`:
{% highlight bash %}
make NAME=led_blinker all
{% endhighlight %}

Building a bootable SD card:
{% highlight bash %}
sudo sh scripts/debian.sh /dev/mmcblk0
{% endhighlight %}

SD card image
-----

Building a bootable SD card image:
{% highlight bash %}
sudo sh scripts/image.sh scripts/debian.sh red-pitaya-debian-8.2-armhf.img
{% endhighlight %}

The SD card image size is 512 MB, so it should fit on any SD card starting from 1 GB.

To write the image to a SD card, the `dd` command-line utility can be used on GNU/Linux and Mac OS X or [Win32 Disk Imager](http://sourceforge.net/projects/win32diskimager/) can be used on MS Windows.

The default password for the `root` account is `changeme`.

A pre-built SD card image can be downloaded from [this link](https://googledrive.com/host/0B-t5klOOymMNfmJ0bFQzTVNXQ3RtWm5SQ2NGTE1hRUlTd3V2emdSNzN6d0pYamNILW83Wmc/red-pitaya-debian-8.2-armhf-20151031.zip).

Resizing SD card partitions on running Red Pitaya:
{% highlight bash %}
# delete second partition
echo -e "d\n2\nw" | fdisk /dev/mmcblk0
# recreate partition
parted -s /dev/mmcblk0 mkpart primary ext4 16MB 100%
# resize partition
resize2fs /dev/mmcblk0p2
{% endhighlight %}

Reprogramming FPGA
-----

It's possible to reprogram the FPGA by loading the bitstream file into `/dev/xdevcfg`:
{% highlight bash %}
cat led_blinker.bit > /dev/xdevcfg
{% endhighlight %}
