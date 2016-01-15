ecosystem=ecosystem-0.92-65-35575ed

rm -rf ${ecosystem}-mcpha

test -f ${ecosystem}.zip || curl -O http://archives.redpitaya.com/devel/${ecosystem}.zip

unzip -d ${ecosystem}-mcpha ${ecosystem}.zip

arm-xilinx-linux-gnueabi-gcc -static projects/mcpha/server/mcpha-server.c -lm -o ${ecosystem}-mcpha/bin/mcpha-server
cp boot.bin devicetree.dtb uImage tmp/mcpha.bit ${ecosystem}-mcpha

cat <<- EOF_CAT > ${ecosystem}-mcpha/uEnv.txt

kernel_image=uImage

devicetree_image=devicetree.dtb

ramdisk_image=uramdisk.image.gz

kernel_load_address=0x2080000

devicetree_load_address=0x2000000

ramdisk_load_address=0x4000000

bootcmd=mmcinfo && fatload mmc 0 \${kernel_load_address} \${kernel_image} && fatload mmc 0 \${devicetree_load_address} \${devicetree_image} && load mmc 0 \${ramdisk_load_address} \${ramdisk_image} && bootm \${kernel_load_address} \${ramdisk_load_address} \${devicetree_load_address}

bootargs=console=ttyPS0,115200 root=/dev/ram rw earlyprintk

EOF_CAT

cat <<- EOF_CAT >> ${ecosystem}-mcpha/etc/init.d/rcS

# start MCPHA server

cat /opt/mcpha.bit > /dev/xdevcfg

/opt/bin/mcpha-server &

EOF_CAT

cd ${ecosystem}-mcpha
zip -r ../${ecosystem}-mcpha.zip .
cd ..
