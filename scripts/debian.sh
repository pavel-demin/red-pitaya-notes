device=$1

boot_dir=`mktemp -d /tmp/BOOT.XXXXXXXXXX`
root_dir=`mktemp -d /tmp/ROOT.XXXXXXXXXX`

linux_dir=tmp/linux-6.6
linux_ver=6.6.32-xilinx

# Choose mirror automatically, depending the geographic and network location
mirror=http://deb.debian.org/debian

distro=bookworm
arch=armhf

passwd=changeme
timezone=Europe/Brussels

# Create partitions

parted -s $device mklabel msdos
parted -s $device mkpart primary fat16 4MiB 16MiB
parted -s $device mkpart primary ext4 16MiB 100%

boot_dev=/dev/`lsblk -ln -o NAME -x NAME $device | sed '2!d'`
root_dev=/dev/`lsblk -ln -o NAME -x NAME $device | sed '3!d'`

# Create file systems

mkfs.vfat -v $boot_dev
mkfs.ext4 -F -j $root_dev

# Mount file systems

mount $boot_dev $boot_dir
mount $root_dev $root_dir

# Copy files to the boot file system

cp boot-rootfs.bin $boot_dir/boot.bin

# Install Debian base system to the root file system

debootstrap --foreign --arch $arch $distro $root_dir $mirror

# Install Linux modules

modules_dir=$root_dir/lib/modules/$linux_ver

mkdir -p $modules_dir/kernel

find $linux_dir -name \*.ko -printf '%P\0' | tar --directory=$linux_dir --owner=0 --group=0 --null --files-from=- -zcf - | tar -zxf - --directory=$modules_dir/kernel

cp $linux_dir/modules.order $linux_dir/modules.builtin $modules_dir/

depmod -a -b $root_dir $linux_ver

# Add missing configuration files and packages

cp /etc/resolv.conf $root_dir/etc/
cp /usr/bin/qemu-arm-static $root_dir/usr/bin/

cp -r debian/etc/apt $root_dir/etc/
cp -r debian/etc/systemd $root_dir/etc/

chroot $root_dir <<- EOF_CHROOT
export LANG=C
export LC_ALL=C
export DEBIAN_FRONTEND=noninteractive
export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

/debootstrap/debootstrap --second-stage

apt-get update
apt-get -y upgrade

apt-get -y install locales

sed -i "/^# en_US.UTF-8 UTF-8$/s/^# //" etc/locale.gen
locale-gen
update-locale LANG=en_US.UTF-8

ln -sf /usr/share/zoneinfo/$timezone etc/localtime
dpkg-reconfigure tzdata

apt-get -y install openssh-server ca-certificates chrony fake-hwclock \
  usbutils psmisc lsof parted curl vim wpasupplicant hostapd dnsmasq \
  firmware-misc-nonfree firmware-realtek firmware-atheros firmware-brcm80211 \
  iw iptables dhcpcd-base ntfs-3g libubootenv-tool

systemctl enable dhcpcd

systemctl disable hostapd
systemctl disable dnsmasq
systemctl disable nftables
systemctl disable wpa_supplicant

sed -i 's/^#PermitRootLogin.*/PermitRootLogin yes/' etc/ssh/sshd_config

sed -i '/^#net.ipv4.ip_forward=1$/s/^#//' etc/sysctl.conf

echo root:$passwd | chpasswd

apt-get clean

service chrony stop
service ssh stop

history -c

sync
EOF_CHROOT

cp -r debian/etc $root_dir/

rm $root_dir/etc/resolv.conf
rm $root_dir/usr/bin/qemu-arm-static

# Unmount file systems

umount $boot_dir $root_dir

rmdir $boot_dir $root_dir

zerofree $root_dev
