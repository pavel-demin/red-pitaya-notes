device=$1

boot_dir=/tmp/BOOT
root_dir=/tmp/ROOT

root_tar=ubuntu-core-14.04.2-core-armhf.tar.gz
root_url=http://cdimage.ubuntu.com/ubuntu-core/releases/14.04/release/$root_tar

passwd=changeme
timezone=Europe/Brussels

# Create partitions

parted -s $device mklabel msdos
parted -s $device mkpart primary fat16 4MB 16MB
parted -s $device mkpart primary ext4 16MB 100%

boot_dev=/dev/`lsblk -lno NAME $device | sed '2!d'`
root_dev=/dev/`lsblk -lno NAME $device | sed '3!d'`

# Create file systems

mkfs.vfat -v $boot_dev
mkfs.ext4 -F -j $root_dev

# Mount file systems

mkdir -p $boot_dir $root_dir

mount $boot_dev $boot_dir
mount $root_dev $root_dir

# Copy files to the boot file system

cp boot.bin devicetree.dtb uEnv.txt uImage $boot_dir

# Copy Ubuntu Core to the root file system

test -f $root_tar || curl -L $root_url -o $root_tar

tar -zxf $root_tar --directory=$root_dir

# Add missing configuration files and packages

cp /etc/resolv.conf $root_dir/etc/
cp /usr/bin/qemu-arm-static $root_dir/usr/bin/

cp patches/fw_env.config $root_dir/etc/
cp fw_printenv $root_dir/usr/bin/fw_printenv
cp fw_printenv $root_dir/usr/bin/fw_setenv

chroot $root_dir <<- EOF_CHROOT
export LANG=C

cat <<- EOF_CAT > etc/apt/apt.conf.d/99norecommends
APT::Install-Recommends "0";
APT::Install-Suggests "0";
EOF_CAT

cat <<- EOF_CAT > etc/fstab
# /etc/fstab: static file system information.
# <file system> <mount point>   <type>  <options>           <dump>  <pass>
/dev/mmcblk0p2  /               ext4    errors=remount-ro   0       1
/dev/mmcblk0p1  /boot           vfat    defaults            0       2
EOF_CAT

cat <<- EOF_CAT >> etc/network/interfaces.d/eth0
auto eth0
iface eth0 inet dhcp
EOF_CAT

cat <<- EOF_CAT >> etc/securetty

# Serial Console for Xilinx Zynq-7000
ttyPS0
EOF_CAT

sed 's/tty1/ttyPS0/g; s/38400/115200/' etc/init/tty1.conf  > etc/init/ttyPS0.conf

echo red-pitaya > etc/hostname

apt-get update
apt-get -y upgrade

apt-get -y install locales

locale-gen en_US.UTF-8
update-locale LANG=en_US.UTF-8

echo $timezone > etc/timezone
dpkg-reconfigure --frontend=noninteractive tzdata

apt-get -y install openssh-server ca-certificates ntp usbutils psmisc lsof \
  parted curl less vim man-db iw wpasupplicant linux-firmware

sed -i 's/^PermitRootLogin.*/PermitRootLogin yes/' etc/ssh/sshd_config

apt-get clean

echo root:$passwd | chpasswd

service ntp stop

history -c
EOF_CHROOT

rm $root_dir/etc/resolv.conf
rm $root_dir/usr/bin/qemu-arm-static

# Unmount file systems

umount $boot_dir $root_dir

rmdir $boot_dir $root_dir
