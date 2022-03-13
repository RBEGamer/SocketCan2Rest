//
// Created by prodevmo on 21.02.22.
//

#ifndef CAN2REST_CAN_ID_FILTER_H
#define CAN2REST_CAN_ID_FILTER_H

#include <map>
#include "SHARED/loguru/loguru.hpp"

class can_id_filter {



public:
    can_id_filter();
    explicit can_id_filter(bool _invert);
    ~can_id_filter();
    void set_invert_filter(bool _invert = false);
    void set_filter(unsigned short int _id, bool _allow);
    bool check_message(unsigned short int _id);
private:
    std::map<const unsigned int,bool> filter_list;
    bool invert = false;
    bool check_key_exists(unsigned short int _id);
};


#endif //CAN2REST_CAN_ID_FILTER_H
