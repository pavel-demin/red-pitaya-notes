ecosystem=ecosystem-0.95-1-6deb253

rm -rf ${ecosystem}-sdr-receiver

test -f ${ecosystem}.zip || curl -O http://downloads.redpitaya.com/downloads/${ecosystem}.zip

unzip -d ${ecosystem}-sdr-receiver ${ecosystem}.zip

arm-linux-gnueabihf-gcc -static projects/sdr_receiver/server/sdr-receiver.c -lm -o ${ecosystem}-sdr-receiver/bin/sdr-receiver
cp tmp/sdr_receiver.bit ${ecosystem}-sdr-receiver

rm -f ${ecosystem}-sdr-receiver/u-boot.scr
cp ${ecosystem}-sdr-receiver/u-boot.scr.buildroot ${ecosystem}-sdr-receiver/u-boot.scr

cat <<- EOF_CAT >> ${ecosystem}-mcpha/etc/network/config

PATH=\$PATH:\$PATH_REDPITAYA/sbin:\$PATH_REDPITAYA/bin

EOF_CAT

cat <<- EOF_CAT >> ${ecosystem}-sdr-receiver/sbin/discovery.sh

# start SDR receiver

devcfg=/sys/devices/soc0/amba/f8007000.devcfg
test -d \$devcfg/fclk/fclk0 || echo fclk0 > \$devcfg/fclk_export
echo 1 > \$devcfg/fclk/fclk0/enable
echo 143000000 > \$devcfg/fclk/fclk0/set_rate

cat /opt/redpitaya/sdr_receiver.bit > /dev/xdevcfg

/opt/redpitaya/bin/sdr-receiver &

EOF_CAT

cd ${ecosystem}-sdr-receiver
zip -r ../${ecosystem}-sdr-receiver.zip .
cd ..
