//
// Created by prodevmo on 21.02.22.
//

#include "can_hashtable.h"



can_hashtable::can_hashtable(){
    //mp.clear();
}

can_hashtable::~can_hashtable() {
    mp.clear();
}


CAN_MESSAGE can_hashtable::get(const unsigned short int _id) {
    if(!check_key_exists(_id)){
        CAN_MESSAGE tmp;
        tmp.valid = false;
        return tmp;
    }

    mutex_get_list.lock();
    CAN_MESSAGE tmp = mp[_id];
    mutex_get_list.unlock();
    tmp.valid = true;
    return tmp;
}

bool can_hashtable::check_key_exists(const unsigned short int _id){
    bool tmp = false;
    mutex_get_list.lock();
    if(mp.find(_id) != mp.end()){
        tmp = true;
    }
    mutex_get_list.unlock();
    return tmp;
}

CAN_MESSAGE can_hashtable::put(const scpp::CanFrame &_frame, const std::chrono::microseconds::rep _timestamp, const unsigned long _sync_tick,const  MESSAGE_SEND _rs) {
    CAN_MESSAGE msg;
    msg.frame = _frame;
    msg.ts = _timestamp;
    msg.sync_tick = _sync_tick;
    msg.message_receieved =_rs;

    //CREATE HASH FROM DATA
    msg.hash = sha256(_frame.data, _frame.len);

#ifdef DONT_WRITE_DUPLICATES
    if(check_key_exists(_frame.id)){
        if(mp[_frame.id].hash != msg.hash){

            mutex_get_list.lock();
            mp[_frame.id] = msg;
            mutex_get_list.unlock();
        }
    }else{
        mutex_get_list.lock();
        mp[_frame.id] = msg;
        mutex_get_list.unlock();
    }
#else
    //INSERT ALWAYS
    mutex_get_list.lock();
    mp[_frame.id] = msg;
    mutex_get_list.unlock();
#endif

    return msg;

}


//ITS USED TO GET AN LIST OF ALLY KEYS OF CURRENT FRAME
//int ITS USED FOR THE JSON CONVERSION
std::vector<int> can_hashtable::get_id_list() {
    std::vector<int> key;

    mutex_get_list.lock();
    for(auto it = mp.begin(); it != mp.end(); ++it) {
        key.push_back((int)it->first);
    }
    mutex_get_list.unlock();
    return key;
}


