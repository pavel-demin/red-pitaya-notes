#! /bin/sh

apps_dir=/media/mmcblk0p1/apps

source $apps_dir/stop.sh

cat $apps_dir/sdr_transceiver_emb/sdr_transceiver_emb.bit > /dev/xdevcfg

cd $apps_dir/sdr_transceiver_emb

if aplay --list-devices | grep 'card 0:'
then
  arecord -fFLOAT_LE -c2 -r48000 | $apps_dir/sdr_transceiver_emb/sdr-transceiver-emb | aplay -fFLOAT_LE -c2 -r48000 &
else
  cat /dev/zero | $apps_dir/sdr_transceiver_emb/sdr-transceiver-emb > /dev/null &
fi

cd -
