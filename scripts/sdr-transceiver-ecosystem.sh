ecosystem=ecosystem-0.95-1-6deb253

rm -rf ${ecosystem}-sdr-transceiver

test -f ${ecosystem}.zip || curl -O http://downloads.redpitaya.com/downloads/${ecosystem}.zip

unzip -d ${ecosystem}-sdr-transceiver ${ecosystem}.zip

arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard projects/sdr_transceiver/server/sdr-transceiver.c -lm -lpthread -o ${ecosystem}-sdr-transceiver/bin/sdr-transceiver
cp tmp/sdr_transceiver.bit ${ecosystem}-sdr-transceiver

rm -f ${ecosystem}-sdr-transceiver/u-boot.scr
cp ${ecosystem}-sdr-transceiver/u-boot.scr.buildroot ${ecosystem}-sdr-transceiver/u-boot.scr

cat <<- EOF_CAT >> ${ecosystem}-mcpha/etc/network/config

PATH=\$PATH:\$PATH_REDPITAYA/sbin:\$PATH_REDPITAYA/bin

EOF_CAT

cat <<- EOF_CAT >> ${ecosystem}-sdr-transceiver/sbin/discovery.sh

# start SDR transceiver

devcfg=/sys/devices/soc0/amba/f8007000.devcfg
test -d \$devcfg/fclk/fclk0 || echo fclk0 > \$devcfg/fclk_export
echo 1 > \$devcfg/fclk/fclk0/enable
echo 143000000 > \$devcfg/fclk/fclk0/set_rate

cat /opt/redpitaya/sdr_transceiver.bit > /dev/xdevcfg

/opt/redpitaya/bin/sdr-transceiver 1 &
/opt/redpitaya/bin/sdr-transceiver 2 &

EOF_CAT

cd ${ecosystem}-sdr-transceiver
zip -r ../${ecosystem}-sdr-transceiver.zip .
cd ..
