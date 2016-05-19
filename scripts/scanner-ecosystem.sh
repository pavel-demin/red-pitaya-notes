ecosystem=ecosystem-0.92-65-35575ed

rm -rf ${ecosystem}-scanner

test -f ${ecosystem}.zip || curl -O http://downloads.redpitaya.com/downloads/0.92/${ecosystem}.zip

unzip -d ${ecosystem}-scanner ${ecosystem}.zip

arm-xilinx-linux-gnueabi-gcc -static projects/scanner/server/scanner.c -lm -o ${ecosystem}-scanner/bin/scanner
cp boot.bin devicetree.dtb uImage tmp/scanner.bit ${ecosystem}-scanner

cat <<- EOF_CAT > ${ecosystem}-scanner/uEnv.txt

kernel_image=uImage

devicetree_image=devicetree.dtb

ramdisk_image=uramdisk.image.gz

kernel_load_address=0x2080000

devicetree_load_address=0x2000000

ramdisk_load_address=0x4000000

bootcmd=mmcinfo && fatload mmc 0 \${kernel_load_address} \${kernel_image} && fatload mmc 0 \${devicetree_load_address} \${devicetree_image} && load mmc 0 \${ramdisk_load_address} \${ramdisk_image} && bootm \${kernel_load_address} \${ramdisk_load_address} \${devicetree_load_address}

bootargs=console=ttyPS0,115200 root=/dev/ram rw earlyprintk

EOF_CAT

cat <<- EOF_CAT >> ${ecosystem}-scanner/etc/init.d/rcS

# start scanner server

cat /opt/scanner.bit > /dev/xdevcfg

/opt/bin/scanner &

EOF_CAT

cd ${ecosystem}-scanner
zip -r ../${ecosystem}-scanner.zip .
cd ..
