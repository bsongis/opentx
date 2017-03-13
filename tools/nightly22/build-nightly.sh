#!/bin/bash

set -e

branch=next
docker=nightly22
workdir=/home/opentx/nightly22
output=/var/www/html/2.2/nightly
version=2.2.0

# Incrementnightly index
index=`cat index.txt`
index=`expr $index + 1`
echo $index > index.txt
suffix="N$index"

cd ${workdir}

# Create on-demand build environment
cp code/radio/util/Dockerfile .
docker build -t new-${docker} --build-arg OPENTX_VERSION_SUFFIX=${suffix} .
docker rmi -f ${docker} || true
docker tag new-${docker} ${docker}
docker rmi -f new-${docker}

# Call sdcard generation
code/tools/nightly22/build-sdcard.sh

# Build Linux companion
docker run -dit --name companion -v /home/opentx/${docker}:/opentx ${docker}
docker exec companion sh -c "mkdir -p build && cd build && cmake /opentx/code && cp radio/src/stamp.h /opentx/binaries/stamp-opentx.txt"
docker exec companion rm -rf build
docker exec companion /opentx/code/tools/build-companion.sh /opentx/code /opentx/binaries/ ${suffix}
docker stop companion
docker rm companion
cp -f  binaries/*.deb ${output}/companion/linux/companion22_${version}${suffix}_amd64.deb

# Request companion compilation on Windows
cd ${output}/companion/windows
wget -qO- http://winbox.open-tx.org/companion-builds/compile22.php?branch=$branch\&suffix=${suffix}
wget -O companion-windows-${version}${suffix}.exe http://winbox.open-tx.org/companion-builds/companion-windows-${version}${suffix}.exe
chmod -Rf g+w companion-windows-${version}${suffix}.exe

# Request companion compilation on Mac OS X
cd ${output}/companion/macosx
wget -qO- http://opentx.blinkt.de:8080/~opentx/build-opentx.py?branch=${branch}\&suffix=${suffix}
wget -O opentx-companion-${version}${suffix}.dmg http://opentx.blinkt.de:8080/~opentx/builds/opentx-companion-${version}${suffix}.dmg
chmod -Rf g+w opentx-companion-${version}${suffix}.dmg

# Clean binaries It will be hosting built on demand firmware
rm -rf $workdir/binaries/*
rm -rf $workdir/binaries/.lock

# Update stamps
cp -f  $workdir/binaries/stamp-opentx.txt ${output}/firmware
rm -f ${output}/companion/windows/companion-windows.stamp
echo "#define VERSION  "'"2.2.0'$suffix'"' >> ${output}/companion/windows/companion-windows.stamp
cp -f ${output}/companion/windows/companion-windows.stamp ${output}/companion/linux/companion-windows.stamp
cp -f ${output}/companion/windows/companion-windows.stamp ${output}/companion/linux/companion-macosx.stamp
