script=$1
image=$2

dd if=/dev/zero of=$image bs=1M count=512

device=`losetup -f`

losetup $device $image

sh $script $device

losetup -d $device
