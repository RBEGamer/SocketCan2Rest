//
// Created by prodevmo on 21.02.22.
//

#ifndef CAN2REST_CAN_MESSAGE_H
#define CAN2REST_CAN_MESSAGE_H

#include <chrono>
#include "SHARED/socketcan-cpp/include/socketcan_cpp/socketcan_cpp.h"
#include "SHARED/magic_enum/include/magic_enum.hpp"



enum class CONVERSATION_TYPE{
    NONE = 0,
    RAW = 1,
    INT = 2,
    BYTE = 3,
    STR = 4,
    BOOL = 5
};


enum class MESSAGE_SEND{
    INVALID = 0,
    SEND = 2,
    GOT = 2
};

struct CAN_MESSAGE{
    scpp::CanFrame frame;
    unsigned long sync_tick = 0;
    std::chrono::microseconds::rep ts;
    std::string hash;
    bool valid;
    MESSAGE_SEND message_receieved = MESSAGE_SEND::INVALID;
    CONVERSATION_TYPE type;
};


#endif //CAN2REST_CAN_MESSAGE_H
