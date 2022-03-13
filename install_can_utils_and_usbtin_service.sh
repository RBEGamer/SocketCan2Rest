#!/bin/bash

if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi

# INSTALL NEEDED TOOLS
sudo apt install automake autoconf libtool cmake -y




echo "INSTALLATION DIR"
echo "${OSMRI_GIT_DIR}"

# INSTALL USBtin RULE
cp ./99-usbtin.rules /etc/udev/rules.d/
udevadm control --reload-rules
udevadm trigger



#  NET-TOOLS
sudo apt install net-tools
# OR USE SOURCE
# cd ./net-tools
#export BINDIR='/usr/bin' SBINDIR='/usr/bin' &&
#yes "" | make -j1                           &&
#make DESTDIR=$PWD/install -j1 install       &&
#rm    install/usr/bin/{nis,yp}domainname    &&
#rm    install/usr/bin/{hostname,dnsdomainname,domainname,ifconfig} &&
#rm -r install/usr/share/man/man1            &&
#rm    install/usr/share/man/man8/ifconfig.8 &&
#unset BINDIR SBINDIR
#chown -R root:root install &&
#cp -a install/* /

# BUILD CAN-UTILS
cd ./can-utils
make
make install
make clean
cd ..

# BUILD LIBSOCKET CAN
pwd
cd libsocketcan
./autogen.sh
./configure
make
make install
make clean
cd ..

# BUILD NANOMSG CAN
#pwd
#cd nanomsg
#cmake .
#make
#make install
#make clean
#cd ..

# BUILD HIREDIS
cd hiredis
make
make install
cd ..

cd redis-plus-plus
rm CMakeCache.txt && rm -Rf CMakeFiles/
cmake -DREDIS_PLUS_PLUS_CXX_STANDARD=17 .
make
make install
rm CMakeCache.txt && rm -Rf CMakeFiles/
cd ..

# BUILD CAN2REST PROGRAM
pwd
cd ./can2rest
rm CMakeCache.txt && rm -Rf CMakeFiles/
cmake .
make
chmod +x ./can2rest
rm -Rf ./CMakeFiles
rm -Rf ./cmake-build-debug
rm -f ./CMakeCache.txt
rm -f ./cmake_install.cmake
rm -f Makefile
cd ..


# CREATE SERVICE FILE
pwd
cp ./can2rest_template.service ./can2rest.service
sed -i -e 's|_OSMRI_GIT_DIR_|'$OSMRI_GIT_DIR'|' ./can2rest.service


# INSTALL SERVICE
cp ./can2rest.service /etc/systemd/system/
cat /etc/systemd/system/can2rest.service
systemctl daemon-reload
# ENABLE SERVICE
systemctl enable can2rest


# MODIFY CAN2REST START FILE
cp ./start_can2rest_template.sh ./start_can2rest.sh
chmod +x ./start_can2rest.sh
sed -i -e 's|_OSMRI_GIT_DIR_|'$OSMRI_GIT_DIR'|' ./start_can2rest.sh
