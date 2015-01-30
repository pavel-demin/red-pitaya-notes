image=$1

dd if=/dev/zero of=$image bs=1M count=512

device=`losetup -f`

losetup $device $image

sh scripts/sdcard.sh $device

losetup -d $device
