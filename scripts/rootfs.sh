mirror=http://ftp.belnet.be/debian
distro=wheezy
rootfs=tmp/rootfs
passwd=changeme
locale=en_US.UTF-8
timezone=Europe/Brussels

# apt-get --no-install-recommends install qemu-user-static debootstrap binfmt-support

mkdir $rootfs
debootstrap --foreign --arch armhf $distro $rootfs $mirror

# cp patches/fw_env.config $rootfs/etc/
# cp fw_printenv $rootfs/usr/bin/fw_printenv
# cp fw_printenv $rootfs/usr/bin/fw_setenv

cp /etc/resolv.conf $rootfs/etc/
cp /usr/bin/qemu-arm-static $rootfs/usr/bin/

chroot $rootfs <<- EOF_CHROOT
export LANG=C

/debootstrap/debootstrap --second-stage

cat <<- EOF_CAT > /etc/apt/sources.list
deb $mirror $distro main contrib non-free
deb-src $mirror $distro main contrib non-free
deb $mirror $distro-updates main contrib non-free
deb-src $mirror $distro-updates main contrib non-free
deb http://security.debian.org/debian-security $distro/updates main contrib non-free
deb-src http://security.debian.org/debian-security $distro/updates main contrib non-free
EOF_CAT

cat <<- EOF_CAT > /etc/apt/apt.conf.d/99norecommends
APT::Install-Recommends "0";
APT::Install-Suggests "0";
EOF_CAT

cat <<- EOF_CAT > /etc/fstab
# /etc/fstab: static file system information.
# <file system> <mount point>   <type>  <options>           <dump>  <pass>
proc            /proc           proc    defaults            0       0
/dev/mmcblk0p2  /               ext4    errors=remount-ro   0       1
/dev/mmcblk0p1  /boot           vfat    defaults            0       2
EOF_CAT

cat <<- EOF_CAT >> /etc/network/interfaces
allow-hotplug eth0
iface eth0 inet dhcp
EOF_CAT

cat <<- EOF_CAT >> /etc/securetty

# Serial Console for Xilinx Zynq-7000
ttyPS0
EOF_CAT

echo T0:2345:respawn:/sbin/getty -L ttyPS0 115200 vt100 >> /etc/inittab

echo red-pitaya > /etc/hostname

apt-get update
apt-get -y upgrade

apt-get -y install openssh-server ntp ntpdate fake-hwclock usbutils curl less locales dialog

sed -i "s/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/" /etc/locale.gen
locale-gen
update-locale LANG=en_US.UTF-8

echo $timezone > /etc/timezone
dpkg-reconfigure --frontend=noninteractive tzdata

apt-get clean

echo root:$passwd | chpasswd

history -c
EOF_CHROOT

rm $rootfs/etc/resolv.conf
rm $rootfs/usr/bin/qemu-arm-static

tar zpcf rootfs.tar.gz $rootfs

