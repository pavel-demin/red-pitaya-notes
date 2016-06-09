ecosystem=ecosystem-0.95-1-6deb253

rm -rf ${ecosystem}-sdr-transceiver-hpsdr

test -f ${ecosystem}.zip || curl -O http://downloads.redpitaya.com/downloads/${ecosystem}.zip

unzip -d ${ecosystem}-sdr-transceiver-hpsdr ${ecosystem}.zip

arm-linux-gnueabihf-gcc -static projects/sdr_transceiver_hpsdr/server/sdr-transceiver-hpsdr.c -D_GNU_SOURCE -lm -lpthread -o ${ecosystem}-sdr-transceiver-hpsdr/bin/sdr-transceiver-hpsdr
cp tmp/sdr_transceiver_hpsdr.bit ${ecosystem}-sdr-transceiver-hpsdr

rm -f ${ecosystem}-sdr-transceiver-hpsdr/u-boot.scr
cp ${ecosystem}-sdr-transceiver-hpsdr/u-boot.scr.buildroot ${ecosystem}-sdr-transceiver-hpsdr/u-boot.scr

cat <<- EOF_CAT >> ${ecosystem}-sdr-transceiver-hpsdr/sbin/discovery.sh

# start SDR transceiver

devcfg=/sys/devices/soc0/amba/f8007000.devcfg
test -d \$devcfg/fclk/fclk0 || echo fclk0 > \$devcfg/fclk_export
echo 1 > \$devcfg/fclk/fclk0/enable
echo 143000000 > \$devcfg/fclk/fclk0/set_rate

cat /opt/redpitaya/sdr_transceiver_hpsdr.bit > /dev/xdevcfg

server=/opt/redpitaya/bin/sdr-transceiver-hpsdr

if aplay --list-devices | grep 'card 0:'
then
  \$server | aplay --format=S16_BE --channels=2 --rate=48000 &
else
  \$server > /dev/null &
fi
EOF_CAT

cd ${ecosystem}-sdr-transceiver-hpsdr
zip -r ../${ecosystem}-sdr-transceiver-hpsdr.zip .
cd ..
