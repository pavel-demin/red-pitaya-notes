# Development machine

The following are the instructions for installing a virtual machine with [Debian](https://www.debian.org/releases/trixie) 13 (amd64) and [Vitis Core Development Kit](https://www.amd.com/en/products/software/adaptive-socs-and-fpgas/vitis.html) 2025.1.

## Creating virtual machine with Debian 13 (amd64)

- Download and install [VirtualBox](https://www.virtualbox.org/wiki/Downloads)

- Download [mini.iso](https://deb.debian.org/debian/dists/trixie/main/installer-amd64/current/images/netboot/mini.iso) for Debian 13

- Start VirtualBox

- Set the experience level to "Expert":

  - From the "File" menu, select "Preferences"

  - Click "Expert"

  - Click "OK"

- Create at least one host-only interface:

  - From the "File" menu, select "Tools" and then "Network Manager"

  - Click the green "Create" icon

- Create a new virtual machine:

  - From the "Machine" menu, select "New"

  - In the "Name and Operating System" section, pick a name for the machine, then set type to "Linux" and subtype to "Debian"

  - In the "Hardware" section, set the memory size to at least 4096 MB

  - In the "Hard Disk" section, set the disk size to at least 256 GB

  - Click "Finish"

- Configure the newly created virtual machine:

  - Select the newly created virtual machine and click the yellow "Settings" icon

  - Select "Network" and enable "Adapter 2" attached to "Host-only Adapter"

  - Set "Adapter Type" to "Paravirtualized Network (virtio-net)" for both "Adapter 1" and "Adapter 2"

  - Select "System" and select only "Optical" in the "Boot Order" list

  - Select "Storage" and select "Empty" under "Controller: IDE"

  - Click the small CD/DVD icon next to the "Optical Drive" drop-down list, select "Choose a Disk File", navigate to the location of the `mini.iso` image, select it, and click "Open"

  - Click "OK"

- Select the newly created virtual machine and click the green "Start" icon

- Press TAB when the installer menu appears

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

- Download "AMD Unified Installer for FPGAs & Adaptive SoCs 2025.1 SFD" from the [Xilinx download page](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vitis/2025-1.html) (the file name is `FPGAs_AdaptiveSoCs_Unified_SDI_2025.1_0530_0145.tar`)

- Create the `/opt/Xilinx` directory, unpack the installer and run it:

```bash
mkdir /opt/Xilinx
tar -xf FPGAs_AdaptiveSoCs_Unified_SDI_2025.1_0530_0145.tar
cd FPGAs_AdaptiveSoCs_Unified_SDI_2025.1_0530_0145
./xsetup
```

- Follow the installation wizard

- Set the installation directory to `/opt/Xilinx`

## Troubleshooting

In recent releases of Linux distributions, the `libtinfo5` package required by `vivado` and `xsct` may be missing, resulting in the following error message when running these programs:
```
libtinfo.so.5: cannot open shared object file: No such file or directory
```
A possible workaround could be to run the following commands, creating symbolic links to the `libtinfo.so.5` library in the appropriate directories:
```
ln -s Ubuntu/24/libtinfo.so.5 /opt/Xilinx/2025.1/Vitis/lib/lnx64.o/libtinfo.so.5
ln -s Ubuntu/24/libtinfo.so.5 /opt/Xilinx/2025.1/Vivado/lib/lnx64.o/libtinfo.so.5
```
