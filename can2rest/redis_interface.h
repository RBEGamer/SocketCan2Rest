//
// Created by Marcel Ochsendorf on 25.02.22.
//

#ifndef CAN2REST_REDIS_INTERFACE_H
#define CAN2REST_REDIS_INTERFACE_H

#include <string>
#include <list>
#include <vector>
//THREAD STUFF
#include <queue>
#include <mutex>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <err.h>

#ifdef ENABLE_REDIS_SUPPORT
#include <sw/redis++/redis++.h>
#endif

#include "SHARED/loguru/loguru.hpp"

#include "can_message.h"
#include "can_frame_to_json.h"

class redis_interface {
public:
    redis_interface();
    void set_connection_details(std::string _redis_host, int _port);
    void set_can_to_json_instance(can_frame_to_json* _frame_json_ptr);
    void store_frame(CAN_MESSAGE _frame);
    void set_field_prefix(std::string _prefix);
    void set_hash_name(std::string _name);
    void enable_service(bool _en);
    void enable_write_to_db(bool _en);

private:

    can_frame_to_json* frame_json_ptr;
    bool enable_write = true;
    std::string redis_host;
    int redis_port;
    std::string hash_name = "CAN_FRAMES";
    std::string redis_field_prefix = "";
    std::mutex cfg_lock;
    std::mutex write_lock;
    std::unordered_map<std::string, std::string> write_map;
    bool thread_running = false;
    std::thread* update_thread = nullptr;
    static void recieve_thread_function(redis_interface* _this);
};


#endif //CAN2REST_REDIS_INTERFACE_H
