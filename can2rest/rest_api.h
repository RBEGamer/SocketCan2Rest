//
// Created by Marcel Ochsendorf on 21.02.22.
//
#pragma once
#ifndef CAN2REST_REST_API_H
#define CAN2REST_REST_API_H

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


#include "SHARED/cpp-httplib/httplib.h"
#include "SHARED/json11/json11.hpp"
#include "SHARED/loguru/loguru.hpp"


#include "can_hashtable.h"
#include "can_frame_to_json.h"
#include "frame_send_queue.h"
#include "config_parser.h"



class rest_api {

public:

    rest_api(can_frame_to_json* _can_to_json_instance, can_hashtable* _can_hashtable_instance, frame_send_queue* _queue);
    void set_bind_settings(std::string _bind_host = "0.0.0.0", int _port = 4242);
    void enable_service(bool _en); //START STOP WEBSERVER THREAD


private:

    bool thread_running = false;

    int port = 4242;
    std::string host =  "0.0.0.0";

    std::thread* update_thread = nullptr;

    can_frame_to_json* can_to_json_instance = nullptr;
    can_hashtable* can_hashtable_instance = nullptr;
    frame_send_queue* queue = nullptr;
    static void recieve_thread_function(rest_api* _this);

};


#endif //CAN2REST_REST_API_H
