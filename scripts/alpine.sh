alpine_url=http://dl-cdn.alpinelinux.org/alpine/v3.14

uboot_tar=alpine-uboot-3.14.0-armv7.tar.gz
uboot_url=$alpine_url/releases/armv7/$uboot_tar

tools_tar=apk-tools-static-2.12.5-r1.apk
tools_url=$alpine_url/main/armv7/$tools_tar

firmware_tar=linux-firmware-other-20210511-r0.apk
firmware_url=$alpine_url/main/armv7/$firmware_tar

linux_dir=tmp/linux-5.10
linux_ver=5.10.46-xilinx

modules_dir=alpine-modloop/lib/modules/$linux_ver

passwd=changeme

test -f $uboot_tar || curl -L $uboot_url -o $uboot_tar
test -f $tools_tar || curl -L $tools_url -o $tools_tar

test -f $firmware_tar || curl -L $firmware_url -o $firmware_tar

for tar in linux-firmware-ath9k_htc-20210511-r0.apk linux-firmware-brcm-20210511-r0.apk linux-firmware-cypress-20210511-r0.apk linux-firmware-rtlwifi-20210511-r0.apk
do
  url=$alpine_url/main/armv7/$tar
  test -f $tar || curl -L $url -o $tar
done

mkdir alpine-uboot
tar -zxf $uboot_tar --directory=alpine-uboot

mkdir alpine-apk
tar -zxf $tools_tar --directory=alpine-apk --warning=no-unknown-keyword

mkdir alpine-initramfs
cd alpine-initramfs

gzip -dc ../alpine-uboot/boot/initramfs-lts | cpio -id
rm -rf etc/modprobe.d
rm -rf lib/firmware
rm -rf lib/modules
rm -rf var
find . | sort | cpio --quiet -o -H newc | gzip -9 > ../initrd.gz

cd ..

mkimage -A arm -T ramdisk -C gzip -d initrd.gz uInitrd

mkdir -p $modules_dir/kernel

find $linux_dir -name \*.ko -printf '%P\0' | tar --directory=$linux_dir --owner=0 --group=0 --null --files-from=- -zcf - | tar -zxf - --directory=$modules_dir/kernel

cp $linux_dir/modules.order $linux_dir/modules.builtin $modules_dir/

depmod -a -b alpine-modloop $linux_ver

tar -zxf $firmware_tar --directory=alpine-modloop/lib/modules --warning=no-unknown-keyword --strip-components=1 --wildcards lib/firmware/ar* lib/firmware/rt*

for tar in linux-firmware-ath9k_htc-20210511-r0.apk linux-firmware-brcm-20210511-r0.apk linux-firmware-cypress-20210511-r0.apk linux-firmware-rtlwifi-20210511-r0.apk
do
  tar -zxf $tar --directory=alpine-modloop/lib/modules --warning=no-unknown-keyword --strip-components=1
done

mksquashfs alpine-modloop/lib modloop -b 1048576 -comp xz -Xdict-size 100%

rm -rf alpine-uboot alpine-initramfs initrd.gz alpine-modloop

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

for project in common_tools led_blinker sdr_receiver_hpsdr sdr_transceiver sdr_transceiver_emb sdr_transceiver_ft8 sdr_transceiver_hpsdr sdr_transceiver_wide sdr_transceiver_wspr mcpha pulsed_nmr vna
do
  mkdir -p $root_dir/media/mmcblk0p1/apps/$project
  cp -r projects/$project/server/* $root_dir/media/mmcblk0p1/apps/$project/
  cp -r projects/$project/app/* $root_dir/media/mmcblk0p1/apps/$project/
  cp tmp/$project.bit $root_dir/media/mmcblk0p1/apps/$project/
done

for project in led_blinker_122_88 sdr_receiver_hpsdr_122_88 sdr_receiver_wide_122_88 sdr_transceiver_122_88 sdr_transceiver_emb_122_88 sdr_transceiver_ft8_122_88 sdr_transceiver_hpsdr_122_88 sdr_transceiver_wspr_122_88 pulsed_nmr_122_88 vna_122_88
do
  mkdir -p $root_dir/media/mmcblk0p1/apps/$project
  cp -r projects/$project/server/* $root_dir/media/mmcblk0p1/apps/$project/
  cp -r projects/$project/app/* $root_dir/media/mmcblk0p1/apps/$project/
  cp tmp/$project.bit $root_dir/media/mmcblk0p1/apps/$project/
done

cp -r alpine-apk/sbin $root_dir/

chroot $root_dir /sbin/apk.static --repository $alpine_url/main --update-cache --allow-untrusted --initdb add alpine-base

echo $alpine_url/main > $root_dir/etc/apk/repositories
echo $alpine_url/community >> $root_dir/etc/apk/repositories

chroot $root_dir /bin/sh <<- EOF_CHROOT

apk update
apk add openssh ucspi-tcp6 iw wpa_supplicant dhcpcd dnsmasq hostapd iptables avahi dbus dcron chrony gpsd libgfortran musl-dev fftw-dev libconfig-dev alsa-lib-dev alsa-utils curl wget less nano bc dos2unix

rc-update add bootmisc boot
rc-update add hostname boot
rc-update add hwdrivers boot
rc-update add modloop boot
rc-update add swclock boot
rc-update add sysctl boot
rc-update add syslog boot
rc-update add urandom boot

rc-update add killprocs shutdown
rc-update add mount-ro shutdown
rc-update add savecache shutdown

rc-update add devfs sysinit
rc-update add dmesg sysinit
rc-update add mdev sysinit

rc-update add avahi-daemon default
rc-update add chronyd default
rc-update add dhcpcd default
rc-update add local default
rc-update add dcron default
rc-update add sshd default

mkdir -p etc/runlevels/wifi
rc-update -s add default wifi

rc-update add iptables wifi
rc-update add dnsmasq wifi
rc-update add hostapd wifi

sed -i 's/^SAVE_ON_STOP=.*/SAVE_ON_STOP="no"/;s/^IPFORWARD=.*/IPFORWARD="yes"/' etc/conf.d/iptables

sed -i 's/^#PermitRootLogin.*/PermitRootLogin yes/' etc/ssh/sshd_config

echo root:$passwd | chpasswd

setup-hostname red-pitaya
hostname red-pitaya

sed -i 's/^# LBU_MEDIA=.*/LBU_MEDIA=mmcblk0p1/' etc/lbu/lbu.conf

cat <<- EOF_CAT > root/.profile
alias rw='mount -o rw,remount /media/mmcblk0p1'
alias ro='mount -o ro,remount /media/mmcblk0p1'
EOF_CAT

ln -s /media/mmcblk0p1/apps root/apps
ln -s /media/mmcblk0p1/wifi root/wifi

lbu add root
lbu delete etc/resolv.conf
lbu delete etc/cron.d/ft8
lbu delete etc/cron.d/ft8_122_88
lbu delete etc/cron.d/wspr
lbu delete etc/cron.d/wspr_122_88
lbu delete root/.ash_history

lbu commit -d

apk add patch make gcc gfortran

wdsp_dir=/media/mmcblk0p1/apps/wdsp
wdsp_tar=/media/mmcblk0p1/apps/wdsp.tar.gz
wdsp_url=https://github.com/pavel-demin/wdsp/archive/master.tar.gz

curl -L \$wdsp_url -o \$wdsp_tar
mkdir -p \$wdsp_dir
tar -zxf \$wdsp_tar --strip-components=1 --directory=\$wdsp_dir
rm \$wdsp_tar
make -C \$wdsp_dir

ft8d_dir=/media/mmcblk0p1/apps/ft8d
ft8d_tar=/media/mmcblk0p1/apps/ft8d.tar.gz
ft8d_url=https://github.com/pavel-demin/ft8d/archive/master.tar.gz

curl -L \$ft8d_url -o \$ft8d_tar
mkdir -p \$ft8d_dir
tar -zxf \$ft8d_tar --strip-components=1 --directory=\$ft8d_dir
rm \$ft8d_tar
make -C \$ft8d_dir

wsprd_dir=/media/mmcblk0p1/apps/wsprd
wsprd_tar=/media/mmcblk0p1/apps/wsprd.tar.gz
wsprd_url=https://github.com/pavel-demin/wsprd/archive/master.tar.gz

curl -L \$wsprd_url -o \$wsprd_tar
mkdir -p \$wsprd_dir
tar -zxf \$wsprd_tar --strip-components=1 --directory=\$wsprd_dir
rm \$wsprd_tar
make -C \$wsprd_dir

for project in common_tools server sdr_receiver_hpsdr sdr_transceiver sdr_transceiver_emb sdr_transceiver_ft8 sdr_transceiver_hpsdr sdr_transceiver_wide sdr_transceiver_wspr mcpha pulsed_nmr vna
do
  make -C /media/mmcblk0p1/apps/\$project clean
  make -C /media/mmcblk0p1/apps/\$project
done

for project in sdr_receiver_hpsdr_122_88 sdr_receiver_wide_122_88 sdr_transceiver_122_88 sdr_transceiver_emb_122_88 sdr_transceiver_ft8_122_88 sdr_transceiver_hpsdr_122_88 sdr_transceiver_wspr_122_88 pulsed_nmr_122_88 vna_122_88
do
  make -C /media/mmcblk0p1/apps/\$project clean
  make -C /media/mmcblk0p1/apps/\$project
done

EOF_CHROOT

cp -r $root_dir/media/mmcblk0p1/apps .
cp -r $root_dir/media/mmcblk0p1/cache .
cp $root_dir/media/mmcblk0p1/red-pitaya.apkovl.tar.gz .

cp -r alpine/wifi .

hostname -F /etc/hostname

rm -rf $root_dir alpine-apk

zip -r red-pitaya-alpine-3.14-armv7-`date +%Y%m%d`.zip apps boot.bin cache devicetree.dtb modloop red-pitaya.apkovl.tar.gz uEnv.txt uImage uInitrd wifi

rm -rf apps cache modloop red-pitaya.apkovl.tar.gz uInitrd wifi
