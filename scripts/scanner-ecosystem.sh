ecosystem=ecosystem-0.95-1-6deb253

rm -rf ${ecosystem}-scanner

test -f ${ecosystem}.zip || curl -O http://downloads.redpitaya.com/downloads/${ecosystem}.zip

unzip -d ${ecosystem}-scanner ${ecosystem}.zip

arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard projects/scanner/server/scanner.c -lm -o ${ecosystem}-scanner/bin/scanner
cp tmp/scanner.bit ${ecosystem}-scanner

rm -f ${ecosystem}-scanner/u-boot.scr
cp ${ecosystem}-scanner/u-boot.scr.buildroot ${ecosystem}-scanner/u-boot.scr

cat <<- EOF_CAT >> ${ecosystem}-scanner/sbin/discovery.sh

# start scanner server

devcfg=/sys/devices/soc0/amba/f8007000.devcfg
test -d \$devcfg/fclk/fclk0 || echo fclk0 > \$devcfg/fclk_export
echo 1 > \$devcfg/fclk/fclk0/enable
echo 143000000 > \$devcfg/fclk/fclk0/set_rate

cat /opt/redpitaya/scanner.bit > /dev/xdevcfg

/opt/redpitaya/bin/scanner &

EOF_CAT

cd ${ecosystem}-scanner
zip -r ../${ecosystem}-scanner.zip .
cd ..
