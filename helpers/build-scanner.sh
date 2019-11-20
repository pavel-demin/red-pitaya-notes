source /opt/Xilinx/Vitis/2019.2/settings64.sh

JOBS=`nproc 2> /dev/null || echo 1`

make -j $JOBS cores

make NAME=led_blinker all

make NAME=scanner bit

sudo sh scripts/alpine-scanner.sh
