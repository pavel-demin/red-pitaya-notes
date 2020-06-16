#! /bin/sh

DIR=`readlink -f $0`
DIR=`dirname $DIR`

date

CORR=`$DIR/measure-corr 30` || exit

for file in *.cfg
do
  sed -i "/^corr/s/=.*/= $CORR;/" $file
done
