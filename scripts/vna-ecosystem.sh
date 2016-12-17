ecosystem=ecosystem-0.95-1-6deb253

rm -rf ${ecosystem}-vna

test -f ${ecosystem}.zip || curl -O http://downloads.redpitaya.com/downloads/${ecosystem}.zip

unzip -d ${ecosystem}-vna ${ecosystem}.zip

arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -ffast-math -fsingle-precision-constant -mvectorize-with-neon-quad projects/vna/server/vna.c -Iprojects/vna/server -lm -lpthread -o ${ecosystem}-vna/bin/vna
cp tmp/vna.bit ${ecosystem}-vna

rm -f ${ecosystem}-vna/u-boot.scr
cp ${ecosystem}-vna/u-boot.scr.buildroot ${ecosystem}-vna/u-boot.scr

cat <<- EOF_CAT >> ${ecosystem}-mcpha/etc/network/config

PATH=\$PATH:\$PATH_REDPITAYA/sbin:\$PATH_REDPITAYA/bin

EOF_CAT

cat <<- EOF_CAT >> ${ecosystem}-vna/sbin/discovery.sh

# start vna server

devcfg=/sys/devices/soc0/amba/f8007000.devcfg
test -d \$devcfg/fclk/fclk0 || echo fclk0 > \$devcfg/fclk_export
echo 1 > \$devcfg/fclk/fclk0/enable
echo 143000000 > \$devcfg/fclk/fclk0/set_rate

cat /opt/redpitaya/vna.bit > /dev/xdevcfg

/opt/redpitaya/bin/vna &

EOF_CAT

cd ${ecosystem}-vna
zip -r ../${ecosystem}-vna.zip .
cd ..
