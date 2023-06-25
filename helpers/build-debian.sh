source /opt/Xilinx/Vitis/2023.1/settings64.sh

DATE=`date +%Y%m%d`

make NAME=led_blinker all

sudo sh scripts/image.sh scripts/debian.sh red-pitaya-debian-11.7-armhf-$DATE.img 1024
zip red-pitaya-debian-11.7-armhf-$DATE.zip red-pitaya-debian-11.7-armhf-$DATE.img
