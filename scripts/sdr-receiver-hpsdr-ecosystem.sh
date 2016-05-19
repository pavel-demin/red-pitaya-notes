ecosystem=ecosystem-0.92-65-35575ed

rm -rf ${ecosystem}-sdr-receiver-hpsdr

test -f ${ecosystem}.zip || curl -O http://downloads.redpitaya.com/downloads/0.92/${ecosystem}.zip

unzip -d ${ecosystem}-sdr-receiver-hpsdr ${ecosystem}.zip

arm-xilinx-linux-gnueabi-gcc -static projects/sdr_receiver_hpsdr/server/sdr-receiver-hpsdr.c -D_GNU_SOURCE -lm -lpthread -o ${ecosystem}-sdr-receiver-hpsdr/bin/sdr-receiver-hpsdr
cp boot.bin devicetree.dtb uImage tmp/sdr_receiver_hpsdr.bit ${ecosystem}-sdr-receiver-hpsdr

cat <<- EOF_CAT > ${ecosystem}-sdr-receiver-hpsdr/uEnv.txt

kernel_image=uImage

devicetree_image=devicetree.dtb

ramdisk_image=uramdisk.image.gz

kernel_load_address=0x2080000

devicetree_load_address=0x2000000

ramdisk_load_address=0x4000000

bootcmd=mmcinfo && fatload mmc 0 \${kernel_load_address} \${kernel_image} && fatload mmc 0 \${devicetree_load_address} \${devicetree_image} && load mmc 0 \${ramdisk_load_address} \${ramdisk_image} && bootm \${kernel_load_address} \${ramdisk_load_address} \${devicetree_load_address}

bootargs=console=ttyPS0,115200 root=/dev/ram rw earlyprintk

EOF_CAT

cat <<- EOF_CAT >> ${ecosystem}-sdr-receiver-hpsdr/etc/init.d/rcS

# start SDR receiver

cat /opt/sdr_receiver_hpsdr.bit > /dev/xdevcfg

/opt/bin/sdr-receiver-hpsdr 1 1 1 1 1 1 &

EOF_CAT

cd ${ecosystem}-sdr-receiver-hpsdr
zip -r ../${ecosystem}-sdr-receiver-hpsdr.zip .
cd ..
