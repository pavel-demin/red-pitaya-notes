ecosystem=ecosystem-0.95-1-6deb253

rm -rf ${ecosystem}-mcpha

test -f ${ecosystem}.zip || curl -O http://downloads.redpitaya.com/downloads/${ecosystem}.zip

unzip -d ${ecosystem}-mcpha ${ecosystem}.zip

arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard projects/mcpha/server/mcpha-server.c -o ${ecosystem}-mcpha/bin/mcpha-server
arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard projects/mcpha/server/pha-server.c -lpthread -o ${ecosystem}-mcpha/bin/pha-server
cp tmp/mcpha.bit ${ecosystem}-mcpha

rm -f ${ecosystem}-mcpha/u-boot.scr
cp ${ecosystem}-mcpha/u-boot.scr.buildroot ${ecosystem}-mcpha/u-boot.scr

cat <<- EOF_CAT >> ${ecosystem}-mcpha/etc/network/config

PATH=\$PATH:\$PATH_REDPITAYA/sbin:\$PATH_REDPITAYA/bin

EOF_CAT

cat <<- EOF_CAT >> ${ecosystem}-mcpha/sbin/discovery.sh

# start mcpha servers

cat /opt/redpitaya/mcpha.bit > /dev/xdevcfg

/opt/redpitaya/bin/mcpha-server &
/opt/redpitaya/bin/pha-server &

EOF_CAT

cd ${ecosystem}-mcpha
zip -r ../${ecosystem}-mcpha.zip .
cd ..
