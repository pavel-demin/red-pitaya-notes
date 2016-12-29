ecosystem=ecosystem-0.95-1-6deb253

rm -rf ${ecosystem}-sdr-transceiver-wide

test -f ${ecosystem}.zip || curl -O http://downloads.redpitaya.com/downloads/${ecosystem}.zip

unzip -d ${ecosystem}-sdr-transceiver-wide ${ecosystem}.zip

arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard projects/sdr_transceiver_wide/server/sdr-transceiver-wide.c -lm -lpthread -o ${ecosystem}-sdr-transceiver-wide/bin/sdr-transceiver-wide
cp tmp/sdr_transceiver_wide.bit ${ecosystem}-sdr-transceiver-wide

rm -f ${ecosystem}-sdr-transceiver-wide/u-boot.scr
cp ${ecosystem}-sdr-transceiver-wide/u-boot.scr.buildroot ${ecosystem}-sdr-transceiver-wide/u-boot.scr

cat <<- EOF_CAT >> ${ecosystem}-sdr-transceiver-wide/etc/network/config

PATH=\$PATH:\$PATH_REDPITAYA/sbin:\$PATH_REDPITAYA/bin

EOF_CAT

cat <<- EOF_CAT >> ${ecosystem}-sdr-transceiver-wide/sbin/discovery.sh

# start SDR transceiver

cat /opt/redpitaya/sdr_transceiver_wide.bit > /dev/xdevcfg

/opt/redpitaya/bin/sdr-transceiver-wide &

EOF_CAT

cd ${ecosystem}-sdr-transceiver-wide
zip -r ../${ecosystem}-sdr-transceiver-wide.zip .
cd ..
