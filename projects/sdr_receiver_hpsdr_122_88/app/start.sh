#! /bin/sh

apps_dir=/media/mmcblk0p1/apps

source $apps_dir/stop.sh

#Check clock freq whether equal to 122.88Mhz

freq=$(fw_printenv clk_freq | sed 's/[^0-9]//g')
if [ "$freq" == "122880000" ]
then
  echo "Freq is 122.88Mhz, HPSDR init"
else
  echo "System will set to 122880000 Hz after auto reboot."
  echo "System will reboot."
  fw_setenv clk_freq 122880000
  reboot
fi

cat $apps_dir/sdr_receiver_hpsdr_122_88/sdr_receiver_hpsdr_122_88.bit > /dev/xdevcfg


$apps_dir/sdr_receiver_hpsdr_122_88/sdr-receiver-hpsdr
