DATE=`date +%Y%m%d`

source /opt/Xilinx/Vivado/2016.4/settings64.sh

make NAME=led_blinker all

sudo sh scripts/image.sh scripts/debian.sh red-pitaya-debian-8.8-armhf-$DATE.img 1024
zip red-pitaya-debian-8.8-armhf-$DATE.zip red-pitaya-debian-8.8-armhf-$DATE.img

sudo sh scripts/image.sh scripts/debian-ecosystem.sh red-pitaya-ecosystem-0.95-debian-8.8-armhf-$DATE.img 1024
zip red-pitaya-ecosystem-0.95-debian-8.8-armhf-$DATE.zip red-pitaya-ecosystem-0.95-debian-8.8-armhf-$DATE.img

make NAME=sdr_transceiver_emb all

sudo sh scripts/image.sh scripts/debian-gnuradio.sh red-pitaya-gnuradio-debian-8.8-armhf-$DATE.img 2048
zip red-pitaya-gnuradio-debian-8.8-armhf-$DATE.zip red-pitaya-gnuradio-debian-8.8-armhf-$DATE.img

make NAME=sdr_transceiver_wspr all

sudo sh scripts/image.sh scripts/debian-wspr.sh red-pitaya-wspr-debian-9.0-armhf-$DATE.img 1024
zip red-pitaya-wspr-debian-9.0-armhf-$DATE.zip red-pitaya-wspr-debian-9.0-armhf-$DATE.img

make NAME=sdr_transceiver tmp/sdr_transceiver.bit
make NAME=sdr_transceiver_hpsdr tmp/sdr_transceiver_hpsdr.bit
make NAME=sdr_receiver_hpsdr tmp/sdr_receiver_hpsdr.bit
make NAME=sdr_transceiver_wide tmp/sdr_transceiver_wide.bit

source helpers/sdr-transceiver-ecosystem.sh
source helpers/sdr-transceiver-bazaar.sh

source helpers/sdr-transceiver-hpsdr-ecosystem.sh
source helpers/sdr-transceiver-hpsdr-bazaar.sh

source helpers/sdr-receiver-hpsdr-ecosystem.sh
source helpers/sdr-receiver-hpsdr-bazaar.sh

source helpers/sdr-transceiver-wide-ecosystem.sh
source helpers/sdr-transceiver-wide-bazaar.sh

make NAME=mcpha tmp/mcpha.bit

source helpers/mcpha-ecosystem.sh
source helpers/mcpha-bazaar.sh

make NAME=pulsed_nmr tmp/pulsed_nmr.bit

source helpers/pulsed-nmr-ecosystem.sh

make NAME=scanner tmp/scanner.bit

source helpers/scanner-ecosystem.sh

make NAME=vna tmp/vna.bit

source helpers/vna-ecosystem.sh
source helpers/vna-bazaar.sh
