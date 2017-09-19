source /opt/Xilinx/Vivado/2016.4/settings64.sh

make NAME=led_blinker all

for project in sdr_receiver_hpsdr sdr_transceiver sdr_transceiver_hpsdr sdr_transceiver_wide sdr_transceiver_wspr mcpha pulsed_nmr scanner vna
do
  make NAME=$project bit
done

sudo sh scripts/alpine.sh

source helpers/sdr-receiver-hpsdr-bazaar.sh
source helpers/sdr-transceiver-bazaar.sh
source helpers/sdr-transceiver-hpsdr-bazaar.sh
source helpers/mcpha-bazaar.sh
source helpers/vna-bazaar.sh
