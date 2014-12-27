parted -s /dev/mmcblk0 mklabel msdos
parted -s /dev/mmcblk0 mkpart primary fat16 4MB 64MB
parted -s /dev/mmcblk0 mkpart primary ext4 64MB 100%

mkfs.vfat -v /dev/mmcblk0p1
mkfs.ext4 -F -j /dev/mmcblk0p2

mkdir /tmp/BOOT
mkdir /tmp/ROOT

mount /dev/mmcblk0p1 /tmp/BOOT
mount /dev/mmcblk0p2 /tmp/ROOT

cp boot.bin devicetree.dtb uEnv.txt uImage /tmp/BOOT
tar zxpf rootfs.tar.gz --strip-components=2 --directory=/tmp/ROOT

umount /tmp/BOOT
umount /tmp/ROOT

rmdir /tmp/BOOT
rmdir /tmp/ROOT

