project=mcpha

cp -a projects/$project/bazaar $project

arm-linux-gnueabihf-gcc -shared -Wall -fPIC -Os -s $project/src/main.c -o $project/controllerhf.so

server=mcpha-server
arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard projects/$project/server/$server.c -lm -lpthread -o $project/$server

server=pha-server
arm-linux-gnueabihf-gcc -static -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard projects/$project/server/$server.c -lpthread -o $project/$server

cp tmp/$project.bit $project

version=1.0-`date +%Y%m%d`
revision=`git log -n1 --pretty=%h`

sed -i "s/REVISION/$revision/; s/VERSION/$version/" $project/info/info.json

zip -r $project-$version.zip $project

rm -rf $project
