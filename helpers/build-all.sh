source /opt/Xilinx/Vivado/2016.4/settings64.sh

make NAME=led_blinker all

for project in sdr_receiver_hpsdr sdr_transceiver sdr_transceiver_hpsdr sdr_transceiver_wspr mcpha vna
do
  make NAME=$project bit
done

sudo sh scripts/alpine.sh
