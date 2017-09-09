#! /bin/sh

rm -f /etc/periodic/wspr

service dcron restart

killall -q decode-wspr.sh write-c2-files wsprd
