ecosystem=ecosystem-0.92-65-35575ed

rm -rf ${ecosystem}-sdr-transceiver

test -f ${ecosystem}.zip || curl -O http://archives.redpitaya.com/devel/${ecosystem}.zip

unzip -d ${ecosystem}-sdr-transceiver ${ecosystem}.zip

arm-xilinx-linux-gnueabi-gcc -static projects/sdr_transceiver/server/sdr-transceiver.c -D_GNU_SOURCE -lm -lpthread -o ${ecosystem}-sdr-transceiver/bin/sdr-transceiver
cp boot.bin devicetree.dtb uImage tmp/sdr_transceiver.bit ${ecosystem}-sdr-transceiver

cat <<- EOF_CAT > ${ecosystem}-sdr-transceiver/uEnv.txt

kernel_image=uImage

devicetree_image=devicetree.dtb

ramdisk_image=uramdisk.image.gz

kernel_load_address=0x2080000

devicetree_load_address=0x2000000

ramdisk_load_address=0x4000000

bootcmd=mmcinfo && fatload mmc 0 \${kernel_load_address} \${kernel_image} && fatload mmc 0 \${devicetree_load_address} \${devicetree_image} && load mmc 0 \${ramdisk_load_address} \${ramdisk_image} && bootm \${kernel_load_address} \${ramdisk_load_address} \${devicetree_load_address}

bootargs=console=ttyPS0,115200 root=/dev/ram rw earlyprintk

EOF_CAT

cat <<- EOF_CAT >> ${ecosystem}-sdr-transceiver/etc/init.d/rcS

# start SDR transceiver

cat /opt/sdr_transceiver.bit > /dev/xdevcfg

/opt/bin/sdr-transceiver 1 &
/opt/bin/sdr-transceiver 2 &

EOF_CAT

cd ${ecosystem}-sdr-transceiver
zip -r ../${ecosystem}-sdr-transceiver.zip .
cd ..
