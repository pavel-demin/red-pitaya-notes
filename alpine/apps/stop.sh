#! /bin/sh

for script in /media/mmcblk0p1/apps/*/stop.sh
do
  $script &
done

wait
