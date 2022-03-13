//
// Created by prodevmo on 22.02.22.
//

#ifndef CAN2REST_FRAME_SEND_QUEUE_H
#define CAN2REST_FRAME_SEND_QUEUE_H

#include <mutex>
#include <queue>

#include "SHARED/hash-library/sha256.h"


#include "can_message.h"
#include "can_frame_to_json.h"
#include "can_hashtable.h"
class frame_send_queue {


public:
    frame_send_queue();


    bool enqueue_int16(unsigned short int _id, short int _value);
    bool enqueue_bytes(unsigned short int _id, std::string _value);
    bool enqueue_bool(unsigned short int _id,bool _value);
    bool enqueue_array(unsigned short int _id, std::vector<unsigned char> _value);

    int message_queued();
    CAN_MESSAGE dequeue();



private:
    SHA256 sha256;
    std::mutex lock;
    std::queue<CAN_MESSAGE> message_queue;
};


#endif //CAN2REST_FRAME_SEND_QUEUE_H
