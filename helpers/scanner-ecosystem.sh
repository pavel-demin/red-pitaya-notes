ecosystem=ecosystem-0.95-1-6deb253

rm -rf ${ecosystem}-scanner

test -f ${ecosystem}.zip || curl -O http://downloads.redpitaya.com/downloads/${ecosystem}.zip

unzip -d ${ecosystem}-scanner ${ecosystem}.zip

arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard projects/scanner/server/scanner.c -lm -o ${ecosystem}-scanner/bin/scanner
cp tmp/scanner.bit ${ecosystem}-scanner

rm -f ${ecosystem}-scanner/u-boot.scr
cp ${ecosystem}-scanner/u-boot.scr.buildroot ${ecosystem}-scanner/u-boot.scr

cat <<- EOF_CAT >> ${ecosystem}-scanner/etc/network/config

PATH=\$PATH:\$PATH_REDPITAYA/sbin:\$PATH_REDPITAYA/bin

EOF_CAT

cat <<- EOF_CAT >> ${ecosystem}-scanner/sbin/discovery.sh

# start scanner server

cat /opt/redpitaya/scanner.bit > /dev/xdevcfg

/opt/redpitaya/bin/scanner &

EOF_CAT

cd ${ecosystem}-scanner
zip -r ../${ecosystem}-scanner.zip .
cd ..
