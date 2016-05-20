ecosystem=ecosystem-0.95-1-6deb253

rm -rf ${ecosystem}-sdr-transceiver-wide

test -f ${ecosystem}.zip || curl -O http://downloads.redpitaya.com/downloads/${ecosystem}.zip

unzip -d ${ecosystem}-sdr-transceiver-wide ${ecosystem}.zip

arm-linux-gnueabihf-gcc -static projects/sdr_transceiver_wide/server/sdr-transceiver-wide.c -lm -lpthread -o ${ecosystem}-sdr-transceiver-wide/bin/sdr-transceiver-wide
cp tmp/sdr_transceiver_wide.bit ${ecosystem}-sdr-transceiver-wide

rm -f ${ecosystem}-sdr-transceiver-wide/u-boot.scr
cp ${ecosystem}-sdr-transceiver-wide/u-boot.scr.buildroot ${ecosystem}-sdr-transceiver-wide/u-boot.scr

cat <<- EOF_CAT >> ${ecosystem}-sdr-transceiver-wide/sbin/discovery.sh

# start SDR transceiver

devcfg=/sys/devices/soc0/amba/f8007000.devcfg
test -d \$devcfg/fclk/fclk0 || echo fclk0 > \$devcfg/fclk_export
echo 1 > \$devcfg/fclk/fclk0/enable
echo 143000000 > \$devcfg/fclk/fclk0/set_rate

cat /opt/redpitaya/sdr_transceiver_wide.bit > /dev/xdevcfg

/opt/redpitaya/bin/sdr-transceiver-wide &

EOF_CAT

cd ${ecosystem}-sdr-transceiver-wide
zip -r ../${ecosystem}-sdr-transceiver-wide.zip .
cd ..
