source /opt/Xilinx/Vitis/2023.1/settings64.sh

JOBS=`nproc 2> /dev/null || echo 1`

make -j $JOBS cores

make NAME=led_blinker all

PRJS="sdr_receiver sdr_receiver_hpsdr sdr_receiver_wide sdr_transceiver sdr_transceiver_ft8 sdr_transceiver_hpsdr sdr_transceiver_wide sdr_transceiver_wspr mcpha pulsed_nmr scanner vna playground template"

printf "%s\n" $PRJS | xargs -n 1 -P $JOBS -I {} make NAME={} bit

PRJS="led_blinker_122_88 sdr_receiver_122_88 sdr_receiver_hpsdr_122_88 sdr_receiver_wide_122_88 sdr_transceiver_122_88 sdr_transceiver_ft8_122_88 sdr_transceiver_hpsdr_122_88 sdr_transceiver_wspr_122_88 pulsed_nmr_122_88 vna_122_88"

printf "%s\n" $PRJS | xargs -n 1 -P $JOBS -I {} make NAME={} PART=xc7z020clg400-1 bit

sudo sh scripts/alpine.sh
