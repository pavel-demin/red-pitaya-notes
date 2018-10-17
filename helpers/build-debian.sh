DATE=`date +%Y%m%d`

source /opt/Xilinx/Vivado/2018.2/settings64.sh

make NAME=led_blinker all

sudo sh scripts/image.sh scripts/debian.sh red-pitaya-debian-8.9-armhf-$DATE.img 1024
zip red-pitaya-debian-8.9-armhf-$DATE.zip red-pitaya-debian-8.9-armhf-$DATE.img

sudo sh scripts/image.sh scripts/debian-ecosystem.sh red-pitaya-ecosystem-0.95-debian-8.9-armhf-$DATE.img 1024
zip red-pitaya-ecosystem-0.95-debian-8.9-armhf-$DATE.zip red-pitaya-ecosystem-0.95-debian-8.9-armhf-$DATE.img

make NAME=sdr_transceiver_emb all

sudo sh scripts/image.sh scripts/debian-gnuradio.sh red-pitaya-gnuradio-debian-8.9-armhf-$DATE.img 2048
zip red-pitaya-gnuradio-debian-8.9-armhf-$DATE.zip red-pitaya-gnuradio-debian-8.9-armhf-$DATE.img

make NAME=sdr_transceiver_wspr all

sudo sh scripts/image.sh scripts/debian-wspr.sh red-pitaya-wspr-debian-9.0-armhf-$DATE.img 1024
zip red-pitaya-wspr-debian-9.0-armhf-$DATE.zip red-pitaya-wspr-debian-9.0-armhf-$DATE.img
