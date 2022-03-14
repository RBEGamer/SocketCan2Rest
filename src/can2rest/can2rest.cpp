#include <string>
#include <iostream>
#include <csignal>
#include <string>
#include <algorithm>
#include <map>
#include <chrono>
#include <ctime>
#include <regex>

#if defined(__MACH__) || defined(__APPLE__)

#include <mach/clock.h>
#include <mach/mach.h>

#endif

//3RD PARTY LIBS
#include "SHARED/loguru/loguru.hpp"
#include "SHARED/socketcan-cpp/include/socketcan_cpp/socketcan_cpp.h"







//LOCAL STUFF
#include "can_hashtable.h"
#include "can_id_filter.h"
#include "can_frame_to_json.h"
#include "rest_api.h"
#include "frame_send_queue.h"
#include "config_parser.h"
#include "redis_interface.h"

//SOME STATIC CONFIG
#ifndef VERSION
#define VERSION "42.42.42"
#endif

#define LOG_FILE_PATH "./can2rest.log"
#define LOG_FILE_PATH_ERROR "./can2rest_error.log"
#define CONFIG_FILE_PATH "./can2rest_config.ini"

#define SYNC_FRAME_ID 42

//DISABLE CAN ON MAC
//ONLY FOR TESTING
#ifdef __APPLE__
#define DISABLE_CAN_SUPPORT
#endif


// GLOBAL VARS
can_hashtable got_can_messages;
can_frame_to_json frame_converter;
can_id_filter id_filter;
frame_send_queue frame_queue;
scpp::SocketCan sockat_can;
rest_api api_server(&frame_converter, &got_can_messages, &frame_queue);
redis_interface redis_api;


unsigned int frame_sync_counter = 0;

volatile bool main_loop_running = false;


void cleanup() {
    LOG_F(INFO, "cleanup");
    sockat_can.close();
    api_server.enable_service(false);
    main_loop_running = false;
}

void signal_callback_handler(int signum) {
    cleanup();
    LOG_F(ERROR, "Caught signal %d\n", signum);
    loguru::flush();
}

bool cmdOptionExists(char **begin, char **end, const std::string &option) {
    return std::find(begin, end, option) != end;
}


int main(int argc, char *argv[]) {

    //REGISTER SIGNAL HANDLER
    signal(SIGINT, signal_callback_handler);


    //SETUP LOGGER
    loguru::init(argc, argv);
    loguru::add_file(LOG_FILE_PATH, loguru::Truncate, loguru::Verbosity_MAX);
    loguru::add_file(LOG_FILE_PATH_ERROR, loguru::Truncate, loguru::Verbosity_WARNING);



    //---- PRINT HELP MESSAGE ------------------------------------------------------ //
    if (cmdOptionExists(argv, argv + argc, "-help")) {
        std::cout << "---- HELP ----" << std::endl;
        std::cout << "-help                   | prints this message" << std::endl;
        std::cout << "-version                | print version of this tool" << std::endl;
        std::cout << "-writeconfig            | creates default config" << std::endl;
        std::cout << "---- END HELP ----" << std::endl;
        return 0;
    }

    //---- PRINT VERSION MESSAGE ------------------------------------------------------ //
    if (cmdOptionExists(argv, argv + argc, "-version")) {
        std::cout << "---- CAN2REST VERSION ----" << std::endl;
        std::cout << "version:" << VERSION << std::endl;
        std::cout << "build date:" << __DATE__ << std::endl;
        std::cout << "build time:" << __TIME__ << std::endl;
        return 0;
    }

    //LOAD CONFIG
    LOG_SCOPE_F(INFO, "LOADING CONFIG FILE %s", CONFIG_FILE_PATH);

    //OVERWRITE WITH EXISTSING CONFIG FILE SETTINGS
    if (!config_parser::getInstance()->loadConfigFile(CONFIG_FILE_PATH) ||
        cmdOptionExists(argv, argv + argc, "-writeconfig")) {
        LOG_F(WARNING, "--- CREATE LOCAL CONFIG FILE -----");
        LOG_F(WARNING, "PROD_V2 HARDWARE MAYBE");
        //LOAD DEFAULTS
        config_parser::getInstance()->loadDefaults();
        //WRITE CONFIG FILE TO FILESYSTEM
        config_parser::getInstance()->createConfigFile(CONFIG_FILE_PATH, true);
        LOG_F(ERROR, "WRITE NEW CONFIGFILE DUE MISSING ONE");
    }
    LOG_F(INFO, "CONFIG FILE LOADED");



    //SETUP CAN FILTER
    //ENABLE/DISABLE PACKET FILTER
    id_filter.set_invert_filter(
            config_parser::getInstance()->getBool_nocheck(config_parser::CFG_ENTRY::REST_WEBSERVER_ENABLE));

    //IF PACKET FILTER ENABLED
    //SET FILTERED IDS HERE
    //id_filter.set_filter(200, true); //TELSAMETER READING
    //id_filter.set_filter(42, true); //SYNC FRAMES

    //FILTER FOR INI CONFIG ENTRIESCAN_FILTER_<ID>=1 ENTRIES
    auto  regex = std::regex("([a-zA-Z])*=can_filter_(\\d)*");
    auto searchResults = std::smatch{};
    //GO THOUGHT ALL INIT CONFIG ENTRIES AND CHECK FOR RIGHT SYNTAX
    for (auto& it: config_parser::getInstance()->get_raw_config_map()) {
        //PERFORM REGEX TEST
        if(std::regex_search(it.first, searchResults, regex)){
            LOG_F(INFO, "%s", it.first.c_str());
            //SPLIT BY _ AND GET THE LAST TOKEN BEFORE LAST _
            const std::string filter_id_str = it.first.substr(it.first.find_last_of("_")+1, it.first.length());
            if(!filter_id_str.empty()){
                //GET ID AND STATE AS INT
                const int filter_id_int = std::atoi(filter_id_str.c_str());
                const int filter_state = std::atoi(it.second.c_str());
                //REGISTER FILTER
                id_filter.set_filter(filter_id_int, filter_state);
            }
        }
    }

    //REGISTER CAN FRAME TYPE CONVERSION HERE
    //ALL MESSAGES WITH CAN ID 200 WILL BE CONVERTED TO 16BIT INT
    frame_converter.register_conversion(200, CONVERSATION_TYPE::INT);

    //FILTER FOR INI CONFIG ENTRIES CAN_CONVERSION_<ID>=INT ENTRIES
    regex = std::regex("([a-zA-Z])*=can_conversion_(\\d)*");
    //GO THOUGHT ALL INIT CONFIG ENTRIES AND CHECK FOR RIGHT SYNTAX
    for (auto& it: config_parser::getInstance()->get_raw_config_map()) {
        //PERFORM REGEX TEST
        if(std::regex_search(it.first, searchResults, regex)){
            LOG_F(INFO, "%s", it.first.c_str());
            //SPLIT BY _ AND GET THE LAST TOKEN BEFORE LAST _
            const std::string filter_id_str = it.first.substr(it.first.find_last_of("_")+1, it.first.length());
            if(!filter_id_str.empty()){
                //GET ID AND STATE AS INT
                const int filter_id_int = std::atoi(filter_id_str.c_str());
                //REGISTER FILTER
                CONVERSATION_TYPE ct = CONVERSATION_TYPE::RAW;
                if(std::string(it.second) == "INT" || std::string(it.second) == "int"){
                    ct = CONVERSATION_TYPE::INT;
                }else if(std::string(it.second) == "STR" || std::string(it.second) == "str"){
                    ct = CONVERSATION_TYPE::STR;
                }else if(std::string(it.second) == "BOOL" || std::string(it.second) == "bool"){
                    ct = CONVERSATION_TYPE::BOOL;
                }else if(std::string(it.second) == "BYTE" || std::string(it.second) == "byte"){
                    ct = CONVERSATION_TYPE::BYTE;
                }

                frame_converter.register_conversion(filter_id_int, ct);
            }
        }
    }

    //SETUP REST API SERVER
    //SET BIND HOST AND PORT
    api_server.set_bind_settings(config_parser::getInstance()->get(config_parser::CFG_ENTRY::REST_WEBSERVER_BIND_HOST),
                                 config_parser::getInstance()->getInt_nocheck(
                                         config_parser::CFG_ENTRY::REST_WEBSERVER_BIND_PORT));
    //ENABLE WEBSERVER
    api_server.enable_service(
            config_parser::getInstance()->getBool_nocheck(config_parser::CFG_ENTRY::REST_WEBSERVER_ENABLE));



    //SETUP REDIS CONNECTION
    redis_api.set_connection_details(config_parser::getInstance()->get(config_parser::CFG_ENTRY::REDIS_HOST),
                                     config_parser::getInstance()->getInt_nocheck(
                                             config_parser::CFG_ENTRY::REDIS_PORT));
    //SET FIELD PREFIX LIKE CAN_<ID> OR CAN_MSG_<ID>
    redis_api.set_field_prefix(config_parser::getInstance()->get(config_parser::CFG_ENTRY::REDIS_FIELD_PREFIX));
    //SET HASH NAME
    redis_api.set_hash_name(config_parser::getInstance()->get(config_parser::CFG_ENTRY::REDIS_HASH_NAME));
    redis_api.enable_write_to_db(true);
    //SET FRMAE TO JSON INSTNACE
    //THIS IS NEEDED TO GET THE TYPE REGISTRATION DATA FOR CORRECT TYPE CONVERSION
    //TO WRITE INTO THE DB
    redis_api.set_can_to_json_instance(&frame_converter);
    //FINALLY ENABLE REDIS
    redis_api.enable_service(
            config_parser::getInstance()->getBool_nocheck(config_parser::CFG_ENTRY::REDIS_ENABLE));
    //START----------------
    loguru::g_stderr_verbosity = 1;
    LOG_SCOPE_F(INFO, "CAN2REST STARTED");
    main_loop_running = true;


#ifndef DISABLE_CAN_SUPPORT
    //SETUP CAN INTERFACE
    const std::string can_if = config_parser::getInstance()->get(config_parser::CFG_ENTRY::CAN_SOCKET_IF_NAME);
    if (can_if.empty() || sockat_can.open(can_if) != scpp::STATUS_OK) {
        LOG_F(ERROR, "Cannot open can socket!");
        cleanup();
        return 1;
    }else{
        LOG_F(INFO, "OPEN CAN SOCKET %s", can_if.c_str());
    }
#endif


    uint32_t last_can_id;
    scpp::CanFrame fr;

    while (main_loop_running) {
#ifndef DISABLE_CAN_SUPPORT
        while (sockat_can.read(fr) == scpp::STATUS_OK) {
            //CHECK IF IT IS ALLOWED MESSAGE
            if (id_filter.check_message(fr.id)) {
                //STORE FRAME
                const CAN_MESSAGE conv_msg = got_can_messages.put(fr, std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count(), frame_sync_counter, MESSAGE_SEND::GOT);



                //STORE MESSAGE TO REDIS
                //IF ENABLED WITH redis.enable_write_to_db(true)
                redis_api.store_frame(conv_msg);

                //IF ITS A SPECIAL SYSTEM SYNC FRAME
                if (fr.id == SYNC_FRAME_ID) {
                    const unsigned int i = (fr.data[1] << 8) | fr.data[0];
                    if (i > frame_sync_counter) {
                        frame_sync_counter = i;
                    }
                }
                //INCREASE SYNC COUNTER
                frame_sync_counter++;
            }
        }
#endif

        //SEND QUEUE
        if (frame_queue.message_queued() > 0) {
            const CAN_MESSAGE msg = frame_queue.dequeue();
            if (msg.valid) {
                //SEND CAN FRAME TO SOCKET
#ifndef DISABLE_CAN_SUPPORT
                auto write_sc_status = sockat_can.write(msg.frame);
                if (write_sc_status != scpp::STATUS_OK){
                    LOG_F(ERROR,"something went wrong on socket write, error code : %d \n", int32_t(write_sc_status));
                }
#endif
                //CHECK IF FRAME TYPE IS REGISTERED
                if (!frame_converter.check_key_exists(msg.frame.id)) {
                    frame_converter.register_conversion(msg.frame.id, msg.type);
                }
                //SAVE FRAME IN LIST
                got_can_messages.put(msg.frame, std::chrono::duration_cast<std::chrono::milliseconds>(
                                             std::chrono::system_clock::now().time_since_epoch()).count(), frame_sync_counter,
                                     msg.message_receieved);

                //STORE MESSAGE TO REDIS
                //IF ENABLED WITH redis.enable_write_to_db(true)
                redis_api.store_frame(msg);
                //SAVE FRAME TO REDIS (IF ENABLED)
                LOG_F(INFO, "frame send with id %i", msg.frame.id);
            }
        }
    }
    return 0;
}
