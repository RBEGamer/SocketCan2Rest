# CAN2REST

The CAN2REST application, creates a bridge between the CAN-Bus (Socket-CAN) and a local network.
It creates a simple bidirectional interface to read and write can messages over a simple REST API.


This set of scripts is used to install the USB to CAN Adapter (USBtin).
It also contains a service to convert the Socket-Can to a usable REST API using a C++ Application.

The REST interface of the CAN2REST is documented in a seperate Postman-Collection `can2rest.postman_collection.json`.


It supports can frame filtering in software and automatic type conversion (int16, bool, byte array, string) bidirectional.


# INSTALLATION

**C++17 IS REQUIRED TO BUILD CAN2REST**

## REQUIRED LIBS

### CAN-UTILS

Not required, but needed for can interface configuratio, if you want to use the can2rest.service.

```bash
cd ./libs && sudo bash ./install_can_utils_and_usbtin.sh
```


## BUILD CAN2REST
```bash

# BUILD
$ cd ./src/can2rest/
$ cmake .
$ make
$ chmod +x ./can2rest

```

# RUN CAN2REST
```bash
# RUN

$ ./can2rest -help
# ---- HELP ----
# -help                   | this message
# -version                | print version
# -writeconfig            | creates default config
# ---- END HELP ----


$ ./can2rest -version
# ---- CAN2REST VERSION ----
# version:0.1.0
# build date:Feb 25 2022
# build time:21:55:51


# CREATE DEFAULT CONFIG FILE
# CREATES ./can2rest_config.ini
$ ./can2rest -writeconfig
$ cat ./can2rest_config.ini
#[SETTINGS]
#CAN_SOCKET_IF_NAME=can0
#REST_WEBSERVER_ENABLE=1
#REST_WEBSERVER_BIND_PORT=4242
#REST_WEBSERVER_BIND_HOST=0.0.0.0
# ...

# NORMAL STARTUP
$ ./can2rest
# 2022-02-25 22:24:18.058 (   0.001s) [main thread     ]             loguru.cpp:611   INFO| stderr verbosity: 0
# 2022-02-25 22:24:18.058 (   0.001s) [main thread     ]             loguru.cpp:612   INFO| -----------------------------------
# 2022-02-25 22:24:18.063 (   0.006s) [main thread     ]             loguru.cpp:770   INFO| Logging to './can2rest.log', mode: 'w', verbosity: 9
# 2022-02-25 22:24:18.063 (   0.006s) [main thread     ]             loguru.cpp:770   INFO| Logging to './can2rest.log', mode: 'w', verbosity: -1
# 2022-02-25 22:24:18.256 (   0.199s) [main thread     ]             can2rest.cpp:124   INFO| { LOADING CONFIG FILE ./can2rest_config.ini
# 2022-02-25 22:24:18.256 (   0.200s) [main thread     ]             config_parser.cpp:104   INFO| config_parser::loadConfigFile WITH FILE ./can2rest_config.ini
# 2022-02-25 22:24:18.257 (   0.200s) [main thread     ]            can2rest.cpp:140   INFO| .   CONFIG FILE LOADED
# 2022-02-25 22:24:18.257 (   0.200s) [main thread     ]            rest_api.cpp:45    INFO| .   rest_api start thread
# 2022-02-25 22:24:18.257 (   0.200s) [main thread     ]            can2rest.cpp:175   INFO| .   { CAN2REST STARTED

```

# CONFIGURATION

```bash
$ $ cat ./can2rest_config.ini
# ENABLE_PACKET_FILTER 
# CAN_FILTER_<ID>=1
# DISABLE_PACKET_FILTER
# CAN_FILTER_<ID>=0
# AUTO_TYPE_CONVERSION 
# CAN_CONVERSION_<ID>=INT
# CAN_CONVERSION_<ID>=STR
# CAN_CONVERSION_<ID>=BOOL
# CAN_CONVERSION_<ID>=BYTE
[SETTINGS]
CAN_SOCKET_IF_NAME=can0
# ENABLE PACKET_FILTER FOR 207 => ONLY FRAMES WITH ID 207 WILL BE PROCESSED
CAN_DISABLE_PACKET_FILTER=0
CAN_FILTER_207=1
# DISABLE PACKET_FILTER
CAN_DISABLE_PACKET_FILTER=1
# ENABLE ATUOMATIC TYPE CONVERSION FOR FRAME ID 207 TO 16BIT INT
CAN_CONVERSION_207=INT
# ENABLE ATUOMATIC TYPE CONVERSION FOR FRAME ID 207 TO BOOL
CAN_CONVERSION_208=BOOL
# REST CONFIG
REST_WEBSERVER_ENABLE=1
REST_WEBSERVER_BIND_PORT=4242
REST_WEBSERVER_BIND_HOST=0.0.0.0
# REDIS CONFIG
REDIS_ENABLE=1
REDIS_HOST=127.0.0.1
REDIS_PORT=6379
REDIS_FIELD_PREFIX=
REDIS_ENABLE_WRITE=0
REDIS_HASH_NAME=CAN_
```


## REST-API USAGE

### GET A RECEIEVED CAN FRAME

```bash
# GET LAST CAN FRAME WITH CAN ID 200
$ curl http://127.0.0.1:4242/receive/200
```

```json
{
    "info": {
        "id": 200,
        "raw_data": [
            192,
            0
        ],
        "raw_length": 2,
        "sync_tick": 4025,
        "type": 2
    },
    "parsed_data": {
        "payload": 192
    }
}
```

If a automatic type conversion is activated for this id:

```c++
//can2rest.cpp
frame_converter.register_conversion(200, can_frame_to_json::CONVERSATION_TYPE::INT);
```

The result contains the `parsed_data` field which contains the parsed result.
`raw_data` entry contains the raw an message buffer, stripped down to the data length.


### GET A LIST OF ALL CAN IDs

```bash
$ curl http://127.0.0.1:4242/list
```

```bash
{
    "can_frame_ids": [
        200,
        242,
        42
    ]
}
```

### SEND A MESSAGE

To send a message, a post request is used with a json body payload.
The type conversion will be applied if a valid json type for `payload` field the is used.

```bash
## SEND STRING TYPE
$ curl -X POST https://reqbin.com/echo/post/json -H 'Content-Type: application/json' -d '{"id": 12,"payload":"hallo"}'

## SEND INT TYPE (16 BIT)
$ curl -X POST https://reqbin.com/echo/post/json -H 'Content-Type: application/json' -d '{"id": 12,"payload":12}'

## SEND BOOL TYPE
$ curl -X POST https://reqbin.com/echo/post/json -H 'Content-Type: application/json' -d '{"id": 12,"payload":true}'

## SEND BYTE ARRAY TYPE
$ curl -X POST https://reqbin.com/echo/post/json -H 'Content-Type: application/json' -d '{"id": 12,"payload":[1,2,3,4,5,6,7,8,255]}'
```





## INSTALL SERVICE

```bash
export CAN2REST_INSTALL_DIR=/home/$USER/can2rest
cd ./service
sudo bash ./install_service.sh
```

## SERVICE TEST

```bash
# PLUG IN USBtin
$ ls /dev/tty*
# /dev/ttyCAN SHOULD EXISTS
$ sudo systemctl start can2rest
$ sudo systemctl status can2rest
```




## FURTHER CAN SETTINGS

### CAN SPEED MODIFICATION

```bash
# init_usbtin.sh
## PARAMETER -s5 => 250kbit or use -S 250000
## PARAMETER -n CAN INTERFACE NAME for e.g. ifconfig
$ slcan_attach -f -n can0 -s5 -o /dev/ttyCAN
```

### VIRTUAL SOCKET-CAN SETUP FOR TESTING

```bash
$ modprobe can-gw
$ ip link add dev can0 type vcan
$ ip link set up can0

# CHECK FOR 
$ ifconfig 
#can0: flags=193<UP,RUNNING,NOARP>  mtu 72
#        unspec 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00  txqueuelen 1000  (UNSPEC)
#        RX packets 0  bytes 0 (0.0 B)
#        RX errors 0  dropped 0  overruns 0  frame 0
#        TX packets 0  bytes 0 (0.0 B)
#        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

