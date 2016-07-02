#!/bin/bash
P_IP_ADDR=192.168.188.21
P_CORR=0
P_FREQ=14100000
P_RATE=500000
FILE=out.txt

timeout --signal=INT 2 ./remote_trx.py --address $P_IP_ADDR --rate $P_RATE --freq=$P_FREQ --corr=$P_CORR > $FILE
actualsize=$(wc -c <"$FILE")
echo "captured $actualsize bytes"
rm -f out.txt
