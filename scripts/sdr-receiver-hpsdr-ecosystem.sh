ecosystem=ecosystem-0.95-1-6deb253

rm -rf ${ecosystem}-sdr-receiver-hpsdr

test -f ${ecosystem}.zip || curl -O http://downloads.redpitaya.com/downloads/${ecosystem}.zip

unzip -d ${ecosystem}-sdr-receiver-hpsdr ${ecosystem}.zip

arm-linux-gnueabihf-gcc -static projects/sdr_receiver_hpsdr/server/sdr-receiver-hpsdr.c -D_GNU_SOURCE -lm -lpthread -o ${ecosystem}-sdr-receiver-hpsdr/bin/sdr-receiver-hpsdr
cp tmp/sdr_receiver_hpsdr.bit ${ecosystem}-sdr-receiver-hpsdr

rm -f ${ecosystem}-sdr-receiver-hpsdr/u-boot.scr
cp ${ecosystem}-sdr-receiver-hpsdr/u-boot.scr.buildroot ${ecosystem}-sdr-receiver-hpsdr/u-boot.scr

cat <<- EOF_CAT >> ${ecosystem}-sdr-receiver-hpsdr/sbin/discovery.sh

# start SDR receiver

devcfg=/sys/devices/soc0/amba/f8007000.devcfg
test -d \$devcfg/fclk/fclk0 || echo fclk0 > \$devcfg/fclk_export
echo 1 > \$devcfg/fclk/fclk0/enable
echo 143000000 > \$devcfg/fclk/fclk0/set_rate

cat /opt/redpitaya/sdr_receiver_hpsdr.bit > /dev/xdevcfg

/opt/redpitaya/bin/sdr-receiver-hpsdr 1 1 1 1 1 1 &

EOF_CAT

cd ${ecosystem}-sdr-receiver-hpsdr
zip -r ../${ecosystem}-sdr-receiver-hpsdr.zip .
cd ..
