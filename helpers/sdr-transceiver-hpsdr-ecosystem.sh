ecosystem=ecosystem-0.95-1-6deb253

rm -rf ${ecosystem}-sdr-transceiver-hpsdr

test -f ${ecosystem}.zip || curl -O http://downloads.redpitaya.com/downloads/${ecosystem}.zip

unzip -d ${ecosystem}-sdr-transceiver-hpsdr ${ecosystem}.zip

arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard projects/sdr_transceiver_hpsdr/server/sdr-transceiver-hpsdr.c -D_GNU_SOURCE -Iprojects/sdr_transceiver_hpsdr/server -lm -lpthread -o ${ecosystem}-sdr-transceiver-hpsdr/bin/sdr-transceiver-hpsdr
cp tmp/sdr_transceiver_hpsdr.bit ${ecosystem}-sdr-transceiver-hpsdr

rm -f ${ecosystem}-sdr-transceiver-hpsdr/u-boot.scr
cp ${ecosystem}-sdr-transceiver-hpsdr/u-boot.scr.buildroot ${ecosystem}-sdr-transceiver-hpsdr/u-boot.scr

cat <<- EOF_CAT >> ${ecosystem}-sdr-transceiver-hpsdr/etc/network/config

PATH=\$PATH:\$PATH_REDPITAYA/sbin:\$PATH_REDPITAYA/bin

EOF_CAT

cat <<- EOF_CAT >> ${ecosystem}-sdr-transceiver-hpsdr/sbin/discovery.sh

# start SDR transceiver

cat /opt/redpitaya/sdr_transceiver_hpsdr.bit > /dev/xdevcfg

/opt/redpitaya/bin/sdr-transceiver-hpsdr &

EOF_CAT

cd ${ecosystem}-sdr-transceiver-hpsdr
zip -r ../${ecosystem}-sdr-transceiver-hpsdr.zip .
cd ..
