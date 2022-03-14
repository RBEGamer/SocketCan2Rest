#!/bin/env bash

cd "$(dirname "$0")"

mkdir -p /tmp/OSMRI/
touch /tmp/OSMRI/CAN_LOCK
while true; do
	FILEA=/tmp/OSMRI/CAN_LOCK
	if test -f "$FILEA"; then
    	echo "STARTING OSMIR CAN2REST"
		_OSMRI_GIT_DIR_/src/src_caninterface/can2rest/can2rest
		sleep 10
	else
	    	exit 1
	fi	
done