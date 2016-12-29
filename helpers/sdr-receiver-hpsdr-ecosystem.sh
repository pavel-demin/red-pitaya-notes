ecosystem=ecosystem-0.95-1-6deb253

rm -rf ${ecosystem}-sdr-receiver-hpsdr

test -f ${ecosystem}.zip || curl -O http://downloads.redpitaya.com/downloads/${ecosystem}.zip

unzip -d ${ecosystem}-sdr-receiver-hpsdr ${ecosystem}.zip

arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard projects/sdr_receiver_hpsdr/server/sdr-receiver-hpsdr.c -D_GNU_SOURCE -lm -lpthread -o ${ecosystem}-sdr-receiver-hpsdr/bin/sdr-receiver-hpsdr
cp tmp/sdr_receiver_hpsdr.bit ${ecosystem}-sdr-receiver-hpsdr

rm -f ${ecosystem}-sdr-receiver-hpsdr/u-boot.scr
cp ${ecosystem}-sdr-receiver-hpsdr/u-boot.scr.buildroot ${ecosystem}-sdr-receiver-hpsdr/u-boot.scr

cat <<- EOF_CAT >> ${ecosystem}-sdr-receiver-hpsdr/etc/network/config

PATH=\$PATH:\$PATH_REDPITAYA/sbin:\$PATH_REDPITAYA/bin

EOF_CAT

cat <<- EOF_CAT >> ${ecosystem}-sdr-receiver-hpsdr/sbin/discovery.sh

# start SDR receiver

cat /opt/redpitaya/sdr_receiver_hpsdr.bit > /dev/xdevcfg

/opt/redpitaya/bin/sdr-receiver-hpsdr 1 1 1 1 1 1 &

EOF_CAT

cd ${ecosystem}-sdr-receiver-hpsdr
zip -r ../${ecosystem}-sdr-receiver-hpsdr.zip .
cd ..
