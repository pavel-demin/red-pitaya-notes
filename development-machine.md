---
layout: page
title: Development machine
permalink: /development-machine/
---

The following are the instructions for installing a virtual machine with [Ubuntu](http://wiki.ubuntu.com/TrustyTahr/ReleaseNotes) 14.04.2 (amd64) and [Vivado Design Suite](http://www.xilinx.com/products/design-tools/vivado) 2015.1 with full SDK.

Creating virtual machine with Ubuntu 14.04.2 (amd64)
-----

- Download and install [VirtualBox](https://www.virtualbox.org/wiki/Downloads)

- Download [mini.iso](http://archive.ubuntu.com/ubuntu/dists/trusty-updates/main/installer-amd64/current/images/netboot/mini.iso) for Ubuntu 14.04

- Start VirtualBox

- Create a new virtual machine using the `mini.iso` image:
  - Click the blue "New" icon

  - Pick a name for the machine, then select "Linux" and "Ubuntu (64 bit)"

  - Set the memory size to at least 2048 MB

  - Select "Create a virtual hard drive now"

  - Select "VDI (VirtualBox Disk Image)"

  - Select "Dynamically allocated"

  - Set the image size to at least 33 GB

  - Select the newly created virtual machine and click the yellow "Settings" icon

  - Select "Network" and enable "Adapter 2" attached to "Host-only Adapter"

  - Set "Adapter Type" to "Paravirtualized Network (virtio-net)" for both "Adapter 1" and "Adapter 2"

  - Select "System" and select only "CD/DVD" in the "Boot Order" list

  - Select "Storage" and select "Empty" below the "IDE Controller"

  - Click the small CD/DVD icon next to the "CD/DVD Drive" drop-down list and select the location of the `mini.iso` image

  - Click "OK"

  - Click the green "Start" icon

  - Press TAB when the "Installer boot menu" appears

  - Edit the boot parameters at the bottom of the boot screen to make them look like the following:

    (the content of the `git.io/FwVS` installation script can be seen at [this link](https://github.com/pavel-demin/red-pitaya-notes/blob/gh-pages/etc/ubuntu.seed))
{% highlight bash %}
linux initrd=initrd.gz url=git.io/FwVS auto=true priority=critical interface=auto
{% endhighlight %}

  - Press ENTER to start the automatic installation

- After installation is done, stop the virtual machine

- Select the newly created virtual machine and click the yellow "Settings" icon

- Select "System" and select only "Hard Disk" in the "Boot Order" list

- Click "OK"

- The virtual machine is ready to use (the root password is "changeme")

Accessing the virtual machine
-----

The virtual machine can be accessed via SSH. To display applications with graphical user interfaces, a X11 server ([Xming](http://sourceforge.net/projects/xming) for MS Windows or [XQuartz](http://xquartz.macosforge.org) for Mac OS X) should be installed on the host computer.

Installing Vivado Design Suite
-----

- Download "Vivado 2015.1: Full Installer for Linux Single File Download Image Including SDK" from the [Xilinx download page](http://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vivado-design-tools/2015-1.html) or from [this direct link](https://secure.xilinx.com/webreg/register.do?group=dlc&version=2015.1&akdm=0&filename=Xilinx_Vivado_SDK_Lin_2015.1_0428_1.tar.gz) (the file name is Xilinx_Vivado_SDK_Lin_2015.1_0428_1.tar.gz)

- Create the `/opt/Xilinx` directory, unpack the installer and run it:
{% highlight bash %}
mkdir /opt/Xilinx
cd /opt/Xilinx
tar -zxf Xilinx_Vivado_SDK_Lin_2015.1_0428_1.tar.gz
cd Xilinx_Vivado_SDK_Lin_2015.1_0428_1
./xsetup
{% endhighlight %}

- Follow the installation wizard and don't forget to select "Software Development Kit" on the installation customization page
  (for detailed information on installation, see [UG973](http://www.xilinx.com/support/documentation/sw_manuals/xilinx2015_1/ug973-vivado-release-notes-install-license.pdf))

Activating Vivado Design Suite license
-----

- Initialize the trusted-storage area for the Xilinx licenses:
{% highlight bash %}
cd /opt/Xilinx/Vivado/2015.1/bin/unwrapped/lnx64.o
./install_fnp.sh
{% endhighlight %}

- Setup Vivado environment:
{% highlight bash %}
source /opt/Xilinx/Vivado/2015.1/settings64.sh
{% endhighlight %}

- Create a license request:
{% highlight bash %}
xlicclientmgr -cr request.xml
{% endhighlight %}

- Open `request.html` in a web browser and activate WebPack or Evaluation or any other Vivado Design Suite license

- Download and copy `Xilinx_License.xml` to the virtual machine

- Process Xilinx_License.xml on the virtual machine:
{% highlight bash %}
xlicclientmgr -p Xilinx_License.xml
{% endhighlight %}
