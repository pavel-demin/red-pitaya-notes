devcfg=/sys/devices/soc0/amba/f8007000.devcfg
test -d $devcfg/fclk/fclk0 || echo fclk0 > $devcfg/fclk_export
echo 1 > $devcfg/fclk/fclk0/enable
echo 143000000 > $devcfg/fclk/fclk0/set_rate

server=/opt/redpitaya/www/apps/sdr_transceiver_hpsdr/sdr-transceiver-hpsdr

if aplay --list-devices | grep 'card 0:'
then
  $server | aplay --format=S16_BE --channels=2 --rate=48000 &
else
  $server > /dev/null &
fi
