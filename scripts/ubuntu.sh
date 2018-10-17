device=$1

boot_dir=`mktemp -d /tmp/BOOT.XXXXXXXXXX`
root_dir=`mktemp -d /tmp/ROOT.XXXXXXXXXX`

linux_dir=tmp/linux-4.14
linux_ver=4.14.76-xilinx

root_tar=ubuntu-base-14.04.5-base-armhf.tar.gz
root_url=http://cdimage.ubuntu.com/ubuntu-base/releases/14.04/release/$root_tar

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

cp boot.bin devicetree.dtb uImage $boot_dir
cp uEnv-ext4.txt $boot_dir/uEnv.txt

# Copy Ubuntu Core to the root file system

test -f $root_tar || curl -L $root_url -o $root_tar

tar -zxf $root_tar --directory=$root_dir

# Install Linux modules

modules_dir=$root_dir/lib/modules/$linux_ver

mkdir -p $modules_dir/kernel

find $linux_dir -name \*.ko -printf '%P\0' | tar --directory=$linux_dir --owner=0 --group=0 --null --files-from=- -zcf - | tar -zxf - --directory=$modules_dir/kernel

cp $linux_dir/modules.order $linux_dir/modules.builtin $modules_dir/

depmod -a -b $root_dir $linux_ver

# Add missing configuration files and packages

cp /etc/resolv.conf $root_dir/etc/
cp /usr/bin/qemu-arm-static $root_dir/usr/bin/

chroot $root_dir <<- EOF_CHROOT
export LANG=C
export LC_ALL=C

export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

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

cat <<- EOF_CAT >> etc/securetty

# Serial Console for Xilinx Zynq-7000
ttyPS0
EOF_CAT

sed 's/tty1/ttyPS0/g; s/38400/115200/' etc/init/tty1.conf > etc/init/ttyPS0.conf

echo red-pitaya > etc/hostname

sed -i '/^# deb .* universe$/s/^# //' etc/apt/sources.list

sed -i '/### END INIT INFO/aexit 0' /etc/init.d/udev
apt-get update
apt-get -y upgrade
sed -i '/### END INIT INFO/{n;d}' /etc/init.d/udev

apt-get -y install locales

locale-gen en_US.UTF-8
update-locale LANG=en_US.UTF-8

echo $timezone > etc/timezone
dpkg-reconfigure --frontend=noninteractive tzdata

apt-get -y install openssh-server ca-certificates ntp usbutils psmisc lsof \
  parted curl less vim man-db iw wpasupplicant linux-firmware ntfs-3g

sed -i 's/^PermitRootLogin.*/PermitRootLogin yes/' etc/ssh/sshd_config

apt-get -y install hostapd isc-dhcp-server iptables

touch etc/udev/rules.d/75-persistent-net-generator.rules

cat <<- EOF_CAT >> etc/network/interfaces.d/eth0
allow-hotplug eth0
iface eth0 inet dhcp
EOF_CAT

cat <<- EOF_CAT > etc/network/interfaces.d/wlan0
allow-hotplug wlan0
iface wlan0 inet static
  address 192.168.42.1
  netmask 255.255.255.0
  post-up service hostapd restart
  post-up service isc-dhcp-server restart
  post-up iptables-restore < /etc/iptables.ipv4.nat
  pre-down iptables-restore < /etc/iptables.ipv4.nonat
  pre-down service isc-dhcp-server stop
  pre-down service hostapd stop
EOF_CAT

cat <<- EOF_CAT > etc/hostapd/hostapd.conf
interface=wlan0
ssid=RedPitaya
driver=nl80211
hw_mode=g
channel=6
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_passphrase=RedPitaya
wpa_key_mgmt=WPA-PSK
wpa_pairwise=CCMP
rsn_pairwise=CCMP
EOF_CAT

cat <<- EOF_CAT > etc/dhcp/dhcpd.conf
ddns-update-style none;
default-lease-time 600;
max-lease-time 7200;
authoritative;
log-facility local7;
subnet 192.168.42.0 netmask 255.255.255.0 {
  range 192.168.42.10 192.168.42.50;
  option broadcast-address 192.168.42.255;
  option routers 192.168.42.1;
  default-lease-time 600;
  max-lease-time 7200;
  option domain-name "local";
  option domain-name-servers 8.8.8.8, 8.8.4.4;
}
EOF_CAT

cat <<- EOF_CAT >> etc/dhcp/dhclient.conf
timeout 20;

lease {
  interface "eth0";
  fixed-address 192.168.1.100;
  option subnet-mask 255.255.255.0;
  renew 2 2030/1/1 00:00:01;
  rebind 2 2030/1/1 00:00:01;
  expire 2 2030/1/1 00:00:01;
}
EOF_CAT

sed -i '/^#net.ipv4.ip_forward=1$/s/^#//' etc/sysctl.conf

cat <<- EOF_CAT > etc/iptables.ipv4.nat
*nat
:PREROUTING ACCEPT [0:0]
:INPUT ACCEPT [0:0]
:OUTPUT ACCEPT [0:0]
:POSTROUTING ACCEPT [0:0]
-A POSTROUTING -o eth0 -j MASQUERADE
COMMIT
*mangle
:PREROUTING ACCEPT [0:0]
:INPUT ACCEPT [0:0]
:FORWARD ACCEPT [0:0]
:OUTPUT ACCEPT [0:0]
:POSTROUTING ACCEPT [0:0]
COMMIT
*filter
:INPUT ACCEPT [0:0]
:FORWARD ACCEPT [0:0]
:OUTPUT ACCEPT [0:0]
-A FORWARD -i eth0 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT
-A FORWARD -i wlan0 -o eth0 -j ACCEPT
COMMIT
EOF_CAT

cat <<- EOF_CAT > etc/iptables.ipv4.nonat
*nat
:PREROUTING ACCEPT [0:0]
:INPUT ACCEPT [0:0]
:OUTPUT ACCEPT [0:0]
:POSTROUTING ACCEPT [0:0]
COMMIT
*mangle
:PREROUTING ACCEPT [0:0]
:INPUT ACCEPT [0:0]
:FORWARD ACCEPT [0:0]
:OUTPUT ACCEPT [0:0]
:POSTROUTING ACCEPT [0:0]
COMMIT
*filter
:INPUT ACCEPT [0:0]
:FORWARD ACCEPT [0:0]
:OUTPUT ACCEPT [0:0]
COMMIT
EOF_CAT

apt-get clean

echo root:$passwd | chpasswd

service ntp stop
service ssh stop

history -c

sync
EOF_CHROOT

rm $root_dir/etc/resolv.conf
rm $root_dir/usr/bin/qemu-arm-static

# Unmount file systems

umount $boot_dir $root_dir

rmdir $boot_dir $root_dir

zerofree $root_dev
