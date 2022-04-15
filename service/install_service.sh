#!/bin/bash



echo "-- INSTALL CAN2REST SERVICE ----"
cd "$(dirname "$0")"

 CWD="$(pwd)"

if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi

if [[ -z "${CAN2REST_INSTALL_DIR}" ]]; then
  export CAN2REST_INSTALL_DIR=/usr/can2rest
fi

echo "INSTALLATION DIR"
echo "${CAN2REST_INSTALL_DIR}"


# CREATE SERVICE FILE
pwd
cp ./can2rest.service.TEMPLATE ./can2rest.service
sed -i -e 's|_CAN2REST_INSTALL_DIR_|'$CAN2REST_INSTALL_DIR'|' ./can2rest.service


# INSTALL SERVICE
cp ./can2rest.service /etc/systemd/system/
cat /etc/systemd/system/can2rest.service
systemctl daemon-reload



# MODIFY CAN2REST START FILE
cp ./start_can2rest.sh.TEMPLATE ./start_can2rest.sh
chmod +x ./start_can2rest.sh
sed -i -e 's|_CAN2REST_INSTALL_DIR_|'$CAN2REST_INSTALL_DIR'|' ./start_can2rest.sh
cat ./start_can2rest.sh



# COPY UDEV RULE
cp ./99-can2rest.rules /etc/udev/rules.d
udevadm control --reload-rules
udevadm trigger
# BUILD
bash ../src/build.sh

echo $CWD
cd $CWD


# FINALLY COPY ALL OVER
mkdir -p $CAN2REST_INSTALL_DIR

FILE=../src/can2rest
if [ -f "$FILE" ]; then
    echo "$FILE exists."
else 
    cd ../src/
    cmake .
    make
    cd ../service/
fi

cp ../src/can2rest $CAN2REST_INSTALL_DIR/can2rest
cp ./start_can2rest.sh $CAN2REST_INSTALL_DIR/start_can2rest.sh
cp ./init_usbtin.sh $CAN2REST_INSTALL_DIR/init_usbtin.sh



# ENABLE SERVICE
systemctl enable can2rest
sleep 10
systemctl status can2rest
