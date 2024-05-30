alpine_url=http://dl-cdn.alpinelinux.org/alpine/v3.20

tools_tar=apk-tools-static-2.14.4-r0.apk
tools_url=$alpine_url/main/armv7/$tools_tar

firmware_tar=linux-firmware-other-20240513-r0.apk
firmware_url=$alpine_url/main/armv7/$firmware_tar

linux_dir=tmp/linux-6.6
linux_ver=6.6.32-xilinx

modules_dir=alpine-modloop/lib/modules/$linux_ver

passwd=changeme

test -f $tools_tar || curl -L $tools_url -o $tools_tar

test -f $firmware_tar || curl -L $firmware_url -o $firmware_tar

for tar in linux-firmware-ath9k_htc-20240513-r0.apk linux-firmware-brcm-20240513-r0.apk linux-firmware-cypress-20240513-r0.apk linux-firmware-rtlwifi-20240513-r0.apk
do
  url=$alpine_url/main/armv7/$tar
  test -f $tar || curl -L $url -o $tar
done

mkdir alpine-apk
tar -zxf $tools_tar --directory=alpine-apk --warning=no-unknown-keyword

mkdir -p $modules_dir/kernel

find $linux_dir -name \*.ko -printf '%P\0' | tar --directory=$linux_dir --owner=0 --group=0 --null --files-from=- -zcf - | tar -zxf - --directory=$modules_dir/kernel

cp $linux_dir/modules.order $linux_dir/modules.builtin $modules_dir/

depmod -a -b alpine-modloop $linux_ver

tar -zxf $firmware_tar --directory=alpine-modloop/lib/modules --warning=no-unknown-keyword --strip-components=1 --wildcards lib/firmware/ar* lib/firmware/rt*

for tar in linux-firmware-ath9k_htc-20240513-r0.apk linux-firmware-brcm-20240513-r0.apk linux-firmware-cypress-20240513-r0.apk linux-firmware-rtlwifi-20240513-r0.apk
do
  tar -zxf $tar --directory=alpine-modloop/lib/modules --warning=no-unknown-keyword --strip-components=1
done

mksquashfs alpine-modloop/lib modloop -b 1048576 -comp xz -Xdict-size 100%

rm -rf alpine-modloop

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

projects="common_tools led_blinker mcpha playground pulsed_nmr sdr_receiver sdr_receiver_hpsdr sdr_receiver_wide sdr_transceiver sdr_transceiver_ft8 sdr_transceiver_hpsdr sdr_transceiver_wide sdr_transceiver_wspr vna"

projects_122_88="led_blinker_122_88 pulsed_nmr_122_88 sdr_receiver_122_88 sdr_receiver_hpsdr_122_88 sdr_receiver_wide_122_88 sdr_transceiver_122_88 sdr_transceiver_ft8_122_88 sdr_transceiver_hpsdr_122_88 sdr_transceiver_wspr_122_88 vna_122_88"

for p in $projects $projects_122_88
do
  mkdir -p $root_dir/media/mmcblk0p1/apps/$p
  cp -r projects/$p/server/* $root_dir/media/mmcblk0p1/apps/$p/
  cp -r projects/$p/app/* $root_dir/media/mmcblk0p1/apps/$p/
  cp tmp/$p.bit $root_dir/media/mmcblk0p1/apps/$p/
done

cp -r alpine-apk/sbin $root_dir/

chroot $root_dir /sbin/apk.static --repository $alpine_url/main --update-cache --allow-untrusted --initdb add alpine-base

echo $alpine_url/main > $root_dir/etc/apk/repositories
echo $alpine_url/community >> $root_dir/etc/apk/repositories

chroot $root_dir /bin/sh <<- EOF_CHROOT

apk update
apk add openssh u-boot-tools ucspi-tcp6 iw wpa_supplicant dhcpcd dnsmasq hostapd iptables avahi dbus dcron chrony gpsd libgfortran musl-dev libconfig-dev alsa-lib-dev alsa-utils curl wget less nano bc dos2unix

rc-update add bootmisc boot
rc-update add hostname boot
rc-update add swclock boot
rc-update add sysctl boot
rc-update add syslog boot
rc-update add seedrng boot

rc-update add killprocs shutdown
rc-update add mount-ro shutdown
rc-update add savecache shutdown

rc-update add devfs sysinit
rc-update add dmesg sysinit
rc-update add mdev sysinit
rc-update add hwdrivers sysinit
rc-update add modloop sysinit

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

apk add make gcc gfortran

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

for p in server $projects $projects_122_88
do
  make -C /media/mmcblk0p1/apps/\$p clean
  make -C /media/mmcblk0p1/apps/\$p
done

EOF_CHROOT

cp -r $root_dir/media/mmcblk0p1/apps .
cp -r $root_dir/media/mmcblk0p1/cache .
cp $root_dir/media/mmcblk0p1/red-pitaya.apkovl.tar.gz .

cp -r alpine/wifi .

hostname -F /etc/hostname

rm -rf $root_dir alpine-apk

zip -r red-pitaya-alpine-3.20-armv7-`date +%Y%m%d`.zip apps boot.bin cache modloop red-pitaya.apkovl.tar.gz wifi

rm -rf apps cache modloop red-pitaya.apkovl.tar.gz wifi
