alpine_url=http://dl-cdn.alpinelinux.org/alpine/v3.6

uboot_tar=alpine-uboot-3.6.2-armhf.tar.gz
uboot_url=$alpine_url/releases/armhf/$uboot_tar

apk_tar=apk-tools-static-2.7.2-r0.apk
apk_url=$alpine_url/main/armhf/$apk_tar

firmware_tar=linux-firmware-20170330-r1.apk
firmware_url=$alpine_url/main/armhf/$firmware_tar

passwd=changeme

test -f $uboot_tar || curl -LO $uboot_url
test -f $apk_tar || curl -LO $apk_url
test -f $firmware_tar || curl -LO $firmware_url

mkdir apks
touch apks/.boot_repository

tar -zxf $firmware_tar --strip-components=1 --wildcards lib/firmware/ar* lib/firmware/ath* lib/firmware/brcm* lib/firmware/ht* lib/firmware/rt* lib/firmware/RT*

mkdir alpine-uboot
tar -zxf $uboot_tar -C alpine-uboot

mkdir alpine-apk
tar -zxf apk-tools-static-2.7.2-r0.apk -C alpine-apk

mkdir alpine-initramfs
cd alpine-initramfs

gzip -dc ../alpine-uboot/boot/initramfs-hardened | cpio -vid
rm -rf etc/modprobe.d
rm -rf lib/firmware
rm -rf lib/modules
find . | sort | cpio --quiet -o -H newc | gzip -9 > ../initrd.gz

cd ..

mkimage -A arm -T ramdisk -C gzip -d initrd.gz uInitrd

rm -rf alpine-uboot alpine-initramfs initrd.gz

root_dir=alpine-root

mkdir -p $root_dir/usr/bin
cp /usr/bin/qemu-arm-static $root_dir/usr/bin/

mkdir -p $root_dir/etc
cp /etc/resolv.conf $root_dir/etc/

mkdir -p $root_dir/etc/apk
mkdir -p $root_dir/media/mmcblk0p1/cache
ln -s /media/mmcblk0p1/cache $root_dir/etc/apk/cache

cp -r alpine/etc $root_dir/
cp -r alpine/apps $root_dir/media/mmcblk0p1/
 
for project in led_blinker sdr_receiver_hpsdr sdr_transceiver sdr_transceiver_hpsdr sdr_transceiver_wspr mcpha vna
do
  mkdir -p $root_dir/media/mmcblk0p1/apps/$project
  cp projects/$project/server/* $root_dir/media/mmcblk0p1/apps/$project/
  cp projects/$project/app/* $root_dir/media/mmcblk0p1/apps/$project/
  cp tmp/$project.bit $root_dir/media/mmcblk0p1/apps/$project/
done

cp -r alpine-apk/sbin $root_dir/

chroot $root_dir /sbin/apk.static --repository $alpine_url/main --update-cache --allow-untrusted --initdb add alpine-base

echo $alpine_url/main > $root_dir/etc/apk/repositories
echo $alpine_url/community >> $root_dir/etc/apk/repositories

chroot $root_dir /bin/sh <<- EOF_CHROOT

apk update
apk add openssh iw wpa_supplicant dhcpcd dnsmasq hostapd iptables dcron chrony gpsd git subversion make gcc musl-dev fftw-dev libconfig-dev curl nano

ln -s /etc/init.d/bootmisc etc/runlevels/boot/bootmisc
ln -s /etc/init.d/hostname etc/runlevels/boot/hostname
ln -s /etc/init.d/swclock etc/runlevels/boot/swclock
ln -s /etc/init.d/sysctl etc/runlevels/boot/sysctl
ln -s /etc/init.d/syslog etc/runlevels/boot/syslog
ln -s /etc/init.d/urandom etc/runlevels/boot/urandom

ln -s /etc/init.d/killprocs etc/runlevels/shutdown/killprocs
ln -s /etc/init.d/mount-ro etc/runlevels/shutdown/mount-ro
ln -s /etc/init.d/savecache etc/runlevels/shutdown/savecache

ln -s /etc/init.d/devfs etc/runlevels/sysinit/devfs
ln -s /etc/init.d/dmesg etc/runlevels/sysinit/dmesg
ln -s /etc/init.d/mdev etc/runlevels/sysinit/mdev

rc-update add wpa_supplicant default
rc-update add dhcpcd default
rc-update add chronyd default
rc-update add inetd default
rc-update add dcron default
rc-update add sshd default

echo root:$passwd | chpasswd

setup-hostname red-pitaya
hostname red-pitaya

sed -i 's/# LBU_MEDIA=.*/LBU_MEDIA=mmcblk0p1/' etc/lbu/lbu.conf

cat <<- EOF_CAT > root/.profile
alias rw='mount -o rw,remount /media/mmcblk0p1'
alias ro='mount -o ro,remount /media/mmcblk0p1'
EOF_CAT

ln -s /media/mmcblk0p1/apps root/apps

for project in server sdr_receiver_hpsdr sdr_transceiver sdr_transceiver_hpsdr mcpha vna
do
  make -C /media/mmcblk0p1/apps/\$project clean
  make -C /media/mmcblk0p1/apps/\$project
done

svn co svn://svn.code.sf.net/p/wsjt/wsjt/branches/wsjtx/lib/wsprd /media/mmcblk0p1/apps/sdr_transceiver_wspr/wsprd
make -C /media/mmcblk0p1/apps/sdr_transceiver_wspr/wsprd CFLAGS='-O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -ffast-math -fsingle-precision-constant -mvectorize-with-neon-quad' wsprd
make -C /media/mmcblk0p1/apps/sdr_transceiver_wspr

lbu add root
lbu delete etc/resolv.conf
lbu delete etc/periodic/wspr
lbu delete root/.ash_history

lbu commit -d

EOF_CHROOT

cp -r $root_dir/media/mmcblk0p1/apps .
cp -r $root_dir/media/mmcblk0p1/cache .
cp -r $root_dir/media/mmcblk0p1/red-pitaya.apkovl.tar.gz .

hostname -F /etc/hostname

rm -rf $root_dir alpine-apk 

zip -r red-pitaya-alpine-3.6-armhf-`date +%Y%m%d`.zip apks apps boot.bin cache devicetree.dtb firmware red-pitaya.apkovl.tar.gz uEnv.txt uImage uInitrd

rm -rf apks apps cache firmware red-pitaya.apkovl.tar.gz uInitrd
