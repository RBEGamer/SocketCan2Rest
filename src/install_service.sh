#!/bin/bash

echo "-- INSTALL CAN2REST SERVICE ----"


if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi

echo "INSTALLATION DIR"
echo "${CAN2RESTDIR}"


# CREATE SERVICE FILE
pwd
cp ./can2rest_template.service ./can2rest.service
sed -i -e 's|_CAN2RESTDIR_|'$CAN2RESTDIR'|' ./can2rest.service


# INSTALL SERVICE
cp ./can2rest.service /etc/systemd/system/
cat /etc/systemd/system/can2rest.service
systemctl daemon-reload
# ENABLE SERVICE
systemctl enable can2rest


# MODIFY CAN2REST START FILE
cp ./start_can2rest_template.sh ./start_can2rest.sh
chmod +x ./start_can2rest.sh
sed -i -e 's|_CAN2RESTDIR_|'$CAN2RESTDIR'|' ./start_can2rest.sh
cat ./start_can2rest.sh
