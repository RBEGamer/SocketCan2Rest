//
// Created by prodevmo on 22.02.22.
//

#include "frame_send_queue.h"

int frame_send_queue::message_queued() {
    lock.lock();
    const int tmp = message_queue.size();
    lock.unlock();
    return tmp;
}

frame_send_queue::frame_send_queue() {

}

bool frame_send_queue::enqueue_int16(const unsigned short int _id,const short int _value) {

    CAN_MESSAGE tmp;

    //ASSEMBLE FRAME
    scpp::CanFrame cf_to_write;
    cf_to_write.id = _id;
    cf_to_write.len = 2;
    //INT TO BYTE ARRAY
    cf_to_write.data[0] =  _value & 0x00FF;
    cf_to_write.data[1] = (_value & 0xff00) >> 8;

    tmp.frame = cf_to_write;
    tmp.message_receieved = MESSAGE_SEND::SEND;
    //tmp.valid = true;
    tmp.hash = sha256(cf_to_write.data, cf_to_write.len);
    tmp.type = CONVERSATION_TYPE::INT; // SET TYPE IMPORTANT FOR AUTO REGISTER FUNCTION IN CAN_HASHTABLE

    //ENQUEUE MESSAGE
    lock.lock();
    message_queue.push(tmp);
    lock.unlock();

    return true;
}

bool frame_send_queue::enqueue_bool(const unsigned short int _id,const bool _value) {
    CAN_MESSAGE tmp;

    //ASSEMBLE FRAME
    scpp::CanFrame cf_to_write;
    cf_to_write.id = _id;
    cf_to_write.len = 1;
    //INT TO BYTE ARRAY
    cf_to_write.data[0] = _value;
    tmp.frame = cf_to_write;
    tmp.message_receieved = MESSAGE_SEND::SEND;
    //tmp.valid = true;
    tmp.hash = "";
    tmp.type = CONVERSATION_TYPE::BOOL; // SET TYPE IMPORTANT FOR AUTO REGISTER FUNCTION IN CAN_HASHTABLE

    //ENQUEUE MESSAGE
    lock.lock();
    message_queue.push(tmp);
    lock.unlock();

    return true;
}


bool frame_send_queue::enqueue_bytes(const unsigned short int _id,const std::string _value) {
    CAN_MESSAGE tmp;

    //DATA LEN MAX 64 BYTES
    int len = _value.length();
    if(len > 64){ len = 64;}
    if(len < 0){ len = 0;}

    //ASSEMBLE FRAME
    scpp::CanFrame cf_to_write;
    cf_to_write.id = _id;
    cf_to_write.len = len;
    //INT TO BYTE ARRAY
    for(int i = 0; i < len; i++){
        cf_to_write.data[i] = (uint8_t)_value.at(i);
    }

    tmp.frame = cf_to_write;
    tmp.message_receieved = MESSAGE_SEND::SEND;
    //tmp.valid = true;
    tmp.hash = "";
    tmp.type = CONVERSATION_TYPE::STR; // SET TYPE IMPORTANT FOR AUTO REGISTER FUNCTION IN CAN_HASHTABLE

    //ENQUEUE MESSAGE
    lock.lock();
    message_queue.push(tmp);
    lock.unlock();

    return true;
}


bool frame_send_queue::enqueue_array(unsigned short int _id, std::vector<unsigned char> _value) {
    CAN_MESSAGE tmp;

    //DATA LEN MAX 64 BYTES
    int sz = _value.size();
    if(sz > 64){ sz = 64;}
    if(sz < 0){ sz = 0;}

    //ASSEMBLE FRAME
    scpp::CanFrame cf_to_write;
    cf_to_write.id = _id;
    cf_to_write.len = sz;
    //INT TO BYTE ARRAY
    for(int i = 0; i < sz; i++){
        cf_to_write.data[i] = _value.at(i);
    }

    tmp.frame = cf_to_write;
    tmp.message_receieved = MESSAGE_SEND::SEND;
    //tmp.valid = true;
    tmp.hash = "";
    tmp.type = CONVERSATION_TYPE::BYTE; // SET TYPE IMPORTANT FOR AUTO REGISTER FUNCTION IN CAN_HASHTABLE

    //ENQUEUE MESSAGE
    lock.lock();
    message_queue.push(tmp);
    lock.unlock();

    return true;
}


CAN_MESSAGE frame_send_queue::dequeue() {
    CAN_MESSAGE tmp;
    tmp.valid = true;

    //CHECK IF MESSAGES IN QUEUE
    const int sz = message_queued();
    if (sz > 0) {
        lock.lock();
        tmp = message_queue.front();
        tmp.valid = true;
        message_queue.pop();
        lock.unlock();
    } else {
        tmp.valid = false;
    }

    return tmp;
}





