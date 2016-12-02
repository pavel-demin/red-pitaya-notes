ecosystem=ecosystem-0.95-1-6deb253

rm -rf ${ecosystem}-mcpha

test -f ${ecosystem}.zip || curl -O http://downloads.redpitaya.com/downloads/${ecosystem}.zip

unzip -d ${ecosystem}-mcpha ${ecosystem}.zip

arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard projects/mcpha/server/mcpha-server.c -lm -o ${ecosystem}-mcpha/bin/mcpha-server
cp tmp/mcpha.bit ${ecosystem}-mcpha

rm -f ${ecosystem}-mcpha/u-boot.scr
cp ${ecosystem}-mcpha/u-boot.scr.buildroot ${ecosystem}-mcpha/u-boot.scr

cat <<- EOF_CAT >> ${ecosystem}-mcpha/sbin/discovery.sh

# start mcpha servers

devcfg=/sys/devices/soc0/amba/f8007000.devcfg
test -d \$devcfg/fclk/fclk0 || echo fclk0 > \$devcfg/fclk_export
echo 1 > \$devcfg/fclk/fclk0/enable
echo 143000000 > \$devcfg/fclk/fclk0/set_rate

cat /opt/redpitaya/mcpha.bit > /dev/xdevcfg

/opt/redpitaya/bin/mcpha-server &
/opt/redpitaya/bin/pha-server &

EOF_CAT

cd ${ecosystem}-mcpha
zip -r ../${ecosystem}-mcpha.zip .
cd ..
