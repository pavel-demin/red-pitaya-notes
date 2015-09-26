ecosystem=ecosystem-0.92-65-35575ed

rm -rf ${ecosystem}-sdr-receiver

test -f ${ecosystem}.zip || curl -O http://archives.redpitaya.com/devel/${ecosystem}.zip

unzip -d ${ecosystem}-sdr-receiver ${ecosystem}.zip

cp sdr-receiver ${ecosystem}-sdr-receiver/bin
cp boot.bin devicetree.dtb sdr_receiver.bin uImage ${ecosystem}-sdr-receiver

cat <<- EOF_CAT > ${ecosystem}-sdr-receiver/uEnv.txt

kernel_image=uImage

devicetree_image=devicetree.dtb

ramdisk_image=uramdisk.image.gz

kernel_load_address=0x2080000

devicetree_load_address=0x2000000

ramdisk_load_address=0x4000000

bootcmd=mmcinfo && fatload mmc 0 \${kernel_load_address} \${kernel_image} && fatload mmc 0 \${devicetree_load_address} \${devicetree_image} && load mmc 0 \${ramdisk_load_address} \${ramdisk_image} && bootm \${kernel_load_address} \${ramdisk_load_address} \${devicetree_load_address}

bootargs=console=ttyPS0,115200 root=/dev/ram rw earlyprintk

EOF_CAT

cat <<- EOF_CAT >> ${ecosystem}-sdr-receiver/etc/init.d/rcS

# start SDR receiver

cat /opt/sdr_receiver.bin > /dev/xdevcfg

/opt/bin/sdr-receiver &

EOF_CAT

cd ${ecosystem}-sdr-receiver
zip -r ../${ecosystem}-sdr-receiver.zip .
cd ..
