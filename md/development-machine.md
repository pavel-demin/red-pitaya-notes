# Development machine

The following are the instructions for installing a virtual machine with [Debian](https://www.debian.org/releases/bookworm) 12 (amd64) and [Vitis Core Development Kit](https://www.amd.com/en/products/software/adaptive-socs-and-fpgas/vitis.html) 2024.2.

## Creating virtual machine with Debian 12 (amd64)

- Download and install [VirtualBox](https://www.virtualbox.org/wiki/Downloads)

- Download [mini.iso](https://deb.debian.org/debian/dists/bookworm/main/installer-amd64/current/images/netboot/mini.iso) for Debian 12

- Start VirtualBox

- Create at least one host-only interface:

  - From the "File" menu select "Host Network Manager"

  - Click the green "Create" icon

  - Click "Close"

- Create a new virtual machine:

  - Click the blue "New" icon

  - Pick a name for the machine, then select "Linux" and "Debian (64 bit)"

  - Set the memory size to at least 4096 MB

  - Select "Create a virtual hard disk now"

  - Select "VDI (VirtualBox Disk Image)"

  - Select "Dynamically allocated"

  - Set the image size to at least 256 GB

  - Select the newly created virtual machine and click the yellow "Settings" icon

  - Select "Network" and enable "Adapter 2" attached to "Host-only Adapter"

  - Set "Adapter Type" to "Paravirtualized Network (virtio-net)" for both "Adapter 1" and "Adapter 2"

  - Select "System" and select only "Optical" in the "Boot Order" list

  - Select "Storage" and select "Empty" below the "IDE Controller"

  - Click the small CD/DVD icon next to the "Optical Drive" drop-down list and select the location of the `mini.iso` image

  - Click "OK"

- Select the newly created virtual machine and click the green "Start" icon

- Press TAB when the "Installer boot menu" appears

- Edit the boot parameters at the bottom of the boot screen to make them look like the following:

  (the content of the `bit.ly/2GH2YHy` installation script can be seen at [this link](https://github.com/pavel-demin/red-pitaya-notes/blob/gh-pages/etc/debian.seed))

```bash
linux initrd=initrd.gz url=bit.ly/2GH2YHy auto=true priority=critical interface=auto
```

- Press ENTER to start the automatic installation

- After installation is done, stop the virtual machine

- Select the newly created virtual machine and click the yellow "Settings" icon

- Select "System" and select only "Hard Disk" in the "Boot Order" list

- Click "OK"

- The virtual machine is ready to use (the default password for the `root` and `red-pitaya` accounts is `changeme`)

## Accessing the virtual machine

The virtual machine can be accessed via SSH. To display applications with graphical user interfaces, a X11 server ([Xming](https://sourceforge.net/projects/xming) for MS Windows or [XQuartz](https://www.xquartz.org) for Mac OS X) should be installed on the host computer. X11 forwarding should be enabled in the SSH client.

## Installing Vitis Core Development Kit

- Download "AMD Unified Installer for FPGAs & Adaptive SoCs 2024.2 SFD " from the [Xilinx download page](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vitis/2024-2.html) (the file name is FPGAs_AdaptiveSoCs_Unified_2024.2_1113_1001.tar)

- Create the `/opt/Xilinx` directory, unpack the installer and run it:

```bash
mkdir /opt/Xilinx
tar -xf FPGAs_AdaptiveSoCs_Unified_2024.2_1113_1001.tar
cd FPGAs_AdaptiveSoCs_Unified_2024.2_1113_1001
./xsetup
```

- Follow the installation wizard

- Set the installation directory to `/opt/Xilinx`
