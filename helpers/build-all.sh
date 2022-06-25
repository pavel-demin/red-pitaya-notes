source /opt/Xilinx/Vitis/2020.2/settings64.sh

JOBS=`nproc 2> /dev/null || echo 1`

make -j $JOBS cores

make NAME=led_blinker all

PRJS="sdr_receiver_hpsdr sdr_receiver_wide sdr_transceiver sdr_transceiver_emb sdr_transceiver_ft8 sdr_transceiver_hpsdr sdr_transceiver_wide sdr_transceiver_wspr mcpha pulsed_nmr scanner vna"

printf "%s\n" $PRJS | xargs -n 1 -P $JOBS -I {} make NAME={} bit

PRJS="led_blinker_122_88 sdr_receiver_hpsdr_122_88 sdr_receiver_wide_122_88 sdr_transceiver_122_88 sdr_transceiver_emb_122_88 sdr_transceiver_ft8_122_88 sdr_transceiver_hpsdr_122_88 sdr_transceiver_wspr_122_88 pulsed_nmr_122_88 vna_122_88"

printf "%s\n" $PRJS | xargs -n 1 -P $JOBS -I {} make NAME={} PART=xc7z020clg400-1 bit

sudo sh scripts/alpine.sh
