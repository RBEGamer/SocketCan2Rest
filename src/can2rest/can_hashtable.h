//
// Created by prodevmo on 21.02.22.
//

#ifndef CAN2REST_HASHTABLE_H
#define CAN2REST_HASHTABLE_H


#include <iostream>
#include <map>
#include <vector>
#include <chrono>
#include <ctime>
#include <string>
#include <mutex>


#include "SHARED/socketcan-cpp/include/socketcan_cpp/socketcan_cpp.h"
#include "SHARED/hash-library/sha256.h"

#include "can_message.h"


#define DONT_WRITE_DUPLICATES

class can_hashtable {

public:



    can_hashtable();
    ~can_hashtable();

    CAN_MESSAGE put(const scpp::CanFrame& _frame, std::chrono::microseconds::rep _timestamp, unsigned long _sync_tick, MESSAGE_SEND _rs);
    CAN_MESSAGE get(unsigned short int _id);
    bool check_key_exists(unsigned short int _id);

    std::vector<int> get_id_list();
private:
    std::map<const unsigned short int, CAN_MESSAGE> mp;
    SHA256 sha256;
    std::mutex mutex_get_list;
};


#endif //CAN2REST_HASHTABLE_H
