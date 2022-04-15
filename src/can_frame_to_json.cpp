//
// Created by prodevmo on 21.02.22.
//

#include "can_frame_to_json.h"


can_frame_to_json::can_frame_to_json() = default;


void can_frame_to_json::register_conversion(const unsigned short int _id, const CONVERSATION_TYPE _type) {
    const std::string type_name = std::string(magic_enum::enum_name(_type));
    LOG_F(INFO, "can_frame_to_json::register_conversion REGISTER FOR ID %i TYPE %s", _id, type_name.c_str());

    lock.lock();
    type_register[_id] = _type;
    lock.unlock();
}


std::string can_frame_to_json::convert_frame(const CAN_MESSAGE _msg) {


    if (!check_key_exists(_msg.frame.id)) {
        LOG_F(WARNING, "can_frame_to_json::convert_frame => CONVERSION  %iNOT REGISTERED", _msg.frame.id);
    }

    lock.lock();
    CONVERSATION_TYPE type = type_register[_msg.frame.id];
    lock.unlock();


#ifdef INTERPRET_NONE_TYPE_FRAMES_AS_RAW
    if (type == CONVERSATION_TYPE::NONE) {
        type = CONVERSATION_TYPE::RAW;
    }
#endif

    // BUILD JSON OBJECT
    //START WITH SOME INFO DATA LIKE ID LENGTH
    const std::string type_name = std::string(magic_enum::enum_name(type));
    const std::string msr = std::string(magic_enum::enum_name(_msg.message_receieved));


    std::string hash = _msg.hash;

    if(hash.empty()){
        hash = sha256(_msg.frame.data, _msg.frame.len);
    }

    const json11::Json info = json11::Json::object{
            {"type",       type_name},
            {"type_raw",   (int) type},
            {"direction", msr},
            {"direction_raw", (int)_msg.message_receieved},
            {"id",         (int) _msg.frame.id},
            {"sync_tick",  (int) _msg.sync_tick},
            {"raw_length", (int) _msg.frame.len},
            {"raw_data",   frame_data_to_vector(const_cast<CAN_MESSAGE &>(_msg))},
            {"payload_hash", hash}
    };

    //PAYLOAD CONTAINS THE TYPE CONVERTED DATA
    //IF THE TYPE WAS REGISTED
    json11::Json data = json11::Json::object{
            {"payload", json11::Json::NUL}
    };

    switch (type) {
        case CONVERSATION_TYPE::NONE:
            LOG_F(WARNING, "can_frame_to_json::convert_frame => CONVERSION FOR ID %i SET TO NONW", _msg.frame.id);
            break;

        case CONVERSATION_TYPE::INT:
            data = json11::Json::object{
                    {"payload", (int) (_msg.frame.data[1] << 8) | _msg.frame.data[0]}
            };
            break;
        case CONVERSATION_TYPE::BOOL:
            data = json11::Json::object{
                    {"payload", (bool) _msg.frame.data[0]}
            };
            break;
        case CONVERSATION_TYPE::BYTE:
            data = json11::Json::object{
                    {"payload", (uint8_t) (_msg.frame.data[0])}
            };
            break;

        case CONVERSATION_TYPE::STR: {
            std::string str(_msg.frame.data, _msg.frame.data + _msg.frame.len);
            data = json11::Json::object{
                    {"payload", str}
            };
        }
            break;
        case CONVERSATION_TYPE::RAW:
            break;

        default:
            LOG_F(WARNING, "can_frame_to_json::convert_frame => CONVERSION INT IMPLEMENTED FOR %i", _msg.frame.id);
            break;
    }

    const json11::Json complete = json11::Json::object{
            {"info",        info},
            {"parsed_data", data}
    };

    return complete.dump();
}

bool can_frame_to_json::check_key_exists(const unsigned short int _id) {
    bool tmp = false;
    lock.lock();
    if (type_register.find(_id) != type_register.end()) {
        tmp = true;
    }
    lock.unlock();
    return tmp;
}


//CONVERTS CAN FRAME DATA ARRAY TO VECTOR OF DATA LEN
std::vector<unsigned short int> can_frame_to_json::frame_data_to_vector(CAN_MESSAGE &_msg) {
#ifdef INCLUDE_COMPLETE_64_BYTE_DATA_IN_JSON
    std::vector<uint8_t> dest;
    dest.insert(dest.begin(), std::begin(_msg.frame.data), std::end(_msg.frame.data));
#else
    std::vector<unsigned short int> dest(_msg.frame.data, _msg.frame.data + _msg.frame.len);
#endif
    return dest;
}
