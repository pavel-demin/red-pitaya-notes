project=sdr_transceiver_hpsdr
server=sdr-transceiver-hpsdr

cp -a projects/$project/bazaar $project
arm-linux-gnueabihf-gcc -shared -Wall -fPIC -Os -s $project/src/main.c -o $project/controllerhf.so
arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard projects/$project/server/$server.c -D_GNU_SOURCE -Iprojects/$project/server -lm -lpthread -o $project/$server
cp tmp/$project.bit $project

version=2.`date +%y-%m%d`
revision=`git log -n1 --pretty=%h`

sed -i "s/REVISION/$revision/; s/VERSION/$version/" $project/info/info.json

zip -r $project-$version.zip $project
