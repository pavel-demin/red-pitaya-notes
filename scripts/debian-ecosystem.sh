device=$1

boot_dir=`mktemp -d /tmp/BOOT.XXXXXXXXXX`
root_dir=`mktemp -d /tmp/ROOT.XXXXXXXXXX`

linux_dir=tmp/linux-5.10
linux_ver=5.10.46-xilinx

ecosystem_tar=red-pitaya-ecosystem-0.95-20160526.tgz
ecosystem_url=https://www.dropbox.com/sh/5fy49wae6xwxa8a/AADrueq0P1OJFy9z6AaJ72nWa/red-pitaya-ecosystem/red-pitaya-ecosystem-0.95-20160526.tgz?dl=1

# Choose mirror automatically, depending the geographic and network location
mirror=http://deb.debian.org/debian

distro=jessie
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

cp boot.bin devicetree.dtb uImage $boot_dir
cp uEnv-ext4.txt $boot_dir/uEnv.txt

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

test -f $ecosystem_tar || curl -L $ecosystem_url -o $ecosystem_tar

mkdir -p $root_dir/var/log/nginx
mkdir -p $root_dir/var/log/redpitaya_nginx
mkdir -p $root_dir/opt
tar -zxf $ecosystem_tar --directory=$root_dir/opt

chroot $root_dir <<- EOF_CHROOT
export LANG=C
export LC_ALL=C

export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

/debootstrap/debootstrap --second-stage

cat <<- EOF_CAT > /etc/apt/sources.list
deb $mirror $distro main contrib non-free
deb-src $mirror $distro main contrib non-free
deb $mirror $distro-updates main contrib non-free
deb-src $mirror $distro-updates main contrib non-free
deb http://security.debian.org/debian-security $distro/updates main contrib non-free
deb-src http://security.debian.org/debian-security $distro/updates main contrib non-free
EOF_CAT

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

echo red-pitaya > etc/hostname

apt-get update
apt-get -y upgrade

apt-get -y install locales

sed -i "/^# en_US.UTF-8 UTF-8$/s/^# //" etc/locale.gen
locale-gen
update-locale LANG=en_US.UTF-8

echo $timezone > etc/timezone
dpkg-reconfigure --frontend=noninteractive tzdata

apt-get -y install openssh-server ca-certificates ntp ntpdate fake-hwclock \
  usbutils psmisc lsof parted curl vim wpasupplicant hostapd isc-dhcp-server \
  iw firmware-realtek firmware-ralink firmware-atheros firmware-brcm80211 \
  build-essential libconfig-dev libpcre3-dev libluajit-5.1-dev libssl-dev \
  libboost-regex1.55-dev libboost-system1.55-dev libboost-thread1.55-dev \
  libcurl4-openssl-dev libcrypto++-dev libfftw3-dev libasound2-dev zlib1g-dev \
  unzip ifplugd ntfs-3g alsa-utils lua-cjson parallel subversion git

sed -i 's/^PermitRootLogin.*/PermitRootLogin yes/' etc/ssh/sshd_config

cat <<- EOF_CAT > etc/systemd/system/redpitaya_nginx.service
[Unit]
Description=Customized Nginx web server for Red Pitaya applications
After=network.target

[Service]
Type=forking
PIDFile=/run/redpitaya_nginx.pid
Environment=PATH_REDPITAYA=/opt/redpitaya
Environment=LD_LIBRARY_PATH=/opt/redpitaya/lib
Environment=PATH=/usr/sbin:/usr/bin:/sbin:/bin:/opt/redpitaya/sbin:/opt/redpitaya/bin
ExecStart=/opt/redpitaya/sbin/nginx -p \\\${PATH_REDPITAYA}/www
ExecReload=/opt/redpitaya/sbin/nginx -p \\\${PATH_REDPITAYA}/www -s reload
ExecStop=/opt/redpitaya/sbin/nginx -p \\\${PATH_REDPITAYA}/www -s quit

[Install]
WantedBy=multi-user.target
EOF_CAT

cat <<- EOF_CAT > etc/systemd/system/redpitaya_scpi.service
[Unit]
Description=SCPI server for Red Pitaya
After=network.target

[Service]
Type=simple
Restart=always
Environment=PATH_REDPITAYA=/opt/redpitaya
Environment=LD_LIBRARY_PATH=/opt/redpitaya/lib
Environment=PATH=/usr/sbin:/usr/bin:/sbin:/bin:/opt/redpitaya/sbin:/opt/redpitaya/bin
ExecStart=/opt/redpitaya/bin/scpi-server
ExecStop=/bin/kill -15 \\\${MAINPID}

[Install]
WantedBy=multi-user.target
EOF_CAT

systemctl enable redpitaya_nginx

cat <<- EOF_CAT > etc/profile.d/red-pitaya.sh
export PATH=\\\$PATH:/opt/redpitaya/bin
export LD_LIBRARY_PATH=\\\$LD_LIBRARY_PATH:/opt/redpitaya/lib
EOF_CAT

touch etc/udev/rules.d/75-persistent-net-generator.rules

cat <<- EOF_CAT > etc/network/interfaces.d/eth0
iface eth0 inet dhcp
EOF_CAT

cat <<- EOF_CAT > etc/default/ifplugd
INTERFACES="eth0"
HOTPLUG_INTERFACES=""
ARGS="-q -f -u0 -d10 -w -I"
SUSPEND_ACTION="stop"
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

cat <<- EOF_CAT > etc/default/hostapd
DAEMON_CONF=/etc/hostapd/hostapd.conf
EOF_CAT

cat <<- EOF_CAT > etc/default/isc-dhcp-server
INTERFACESv4=wlan0
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
