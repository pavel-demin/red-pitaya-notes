ecosystem=ecosystem-0.92-65-35575ed

rm -rf ${ecosystem}-vna

test -f ${ecosystem}.zip || curl -O http://downloads.redpitaya.com/downloads/0.92/${ecosystem}.zip

unzip -d ${ecosystem}-vna ${ecosystem}.zip

arm-xilinx-linux-gnueabi-gcc -static projects/vna/server/vna.c -lm -lpthread -o ${ecosystem}-vna/bin/vna
cp boot.bin devicetree.dtb uImage tmp/vna.bit ${ecosystem}-vna

cat <<- EOF_CAT > ${ecosystem}-vna/uEnv.txt

kernel_image=uImage

devicetree_image=devicetree.dtb

ramdisk_image=uramdisk.image.gz

kernel_load_address=0x2080000

devicetree_load_address=0x2000000

ramdisk_load_address=0x4000000

bootcmd=mmcinfo && fatload mmc 0 \${kernel_load_address} \${kernel_image} && fatload mmc 0 \${devicetree_load_address} \${devicetree_image} && load mmc 0 \${ramdisk_load_address} \${ramdisk_image} && bootm \${kernel_load_address} \${ramdisk_load_address} \${devicetree_load_address}

bootargs=console=ttyPS0,115200 root=/dev/ram rw earlyprintk

EOF_CAT

cat <<- EOF_CAT >> ${ecosystem}-vna/etc/init.d/rcS

# start vna server

cat /opt/vna.bit > /dev/xdevcfg

/opt/bin/vna &

EOF_CAT

cd ${ecosystem}-vna
zip -r ../${ecosystem}-vna.zip .
cd ..
