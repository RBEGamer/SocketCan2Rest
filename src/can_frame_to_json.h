//
// Created by prodevmo on 21.02.22.
//

#ifndef CAN2REST_CAN_FRAME_TO_JSON_H
#define CAN2REST_CAN_FRAME_TO_JSON_H

#include <map>
#include <string>
#include <vector>
#include <mutex>

#include "SHARED/loguru/loguru.hpp"
#include "SHARED/json11/json11.hpp"
#include "SHARED/magic_enum/include/magic_enum.hpp"
#include "SHARED/hash-library/sha256.h"

#include "can_message.h"
#include "can_hashtable.h"




//#define INCLUDE_COMPLETE_64_BYTE_DATA_IN_JSON
#define INTERPRET_NONE_TYPE_FRAMES_AS_RAW

class can_frame_to_json {

public:





    can_frame_to_json();


    void register_conversion(unsigned short int _id, CONVERSATION_TYPE _type);
    std::string convert_frame(CAN_MESSAGE _msg);
    bool check_key_exists(unsigned short int _id);
    static std::vector<unsigned short int> frame_data_to_vector(CAN_MESSAGE& _msg);



private:
    SHA256 sha256;

    std::map<const unsigned short int, CONVERSATION_TYPE> type_register;
    std::mutex lock;

};


#endif //CAN2REST_CAN_FRAME_TO_JSON_H
