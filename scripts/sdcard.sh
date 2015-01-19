device=$1

boot_dir=/tmp/BOOT
root_dir=/tmp/ROOT

parted -s $device mklabel msdos
parted -s $device mkpart primary fat16 4MB 16MB
parted -s $device mkpart primary ext4 16MB 100%

boot_dev=/dev/`lsblk -lno NAME $device | sed '2!d'`
root_dev=/dev/`lsblk -lno NAME $device | sed '3!d'`

mkfs.vfat -v $boot_dev
mkfs.ext4 -F -j $root_dev

mkdir $boot_dir $root_dir

mount $boot_dev $boot_dir
mount $root_dev $root_dir

cp boot.bin devicetree.dtb uEnv.txt uImage $boot_dir
tar --numeric-owner -zxpf rootfs.tar.gz --strip-components=2 --directory=$root_dir

umount $boot_dir $root_dir

rmdir $boot_dir $root_dir
