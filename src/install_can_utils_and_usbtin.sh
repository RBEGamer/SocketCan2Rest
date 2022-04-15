#!/bin/bash

if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi

# INSTALL NEEDED TOOLS
sudo apt install automake autoconf libtool cmake -y


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