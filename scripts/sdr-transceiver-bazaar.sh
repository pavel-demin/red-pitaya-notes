project=sdr_transceiver

cp -a projects/$project/bazaar $project
arm-linux-gnueabihf-gcc -shared -Wall -fPIC -lstdc++ -Os -s $project/src/main.c -o $project/controllerhf.so
cp tmp/$project.bit $project

build_number=`git rev-list HEAD --count`
revision=`git log -n1 --pretty=%h`

sed -i "s/REVISION/$revision/; s/BUILD_NUMBER/$build_number/" $project/info/info.json

zip -r $project-0.94-$build_number.zip $project
