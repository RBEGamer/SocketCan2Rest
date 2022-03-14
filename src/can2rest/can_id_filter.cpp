//
// Created by prodevmo on 21.02.22.
//

#include "can_id_filter.h"

can_id_filter::can_id_filter() = default;

can_id_filter::can_id_filter(const bool _invert) {
    set_invert_filter(_invert);
}

void can_id_filter::set_invert_filter(const bool _invert){
    invert = _invert;
}
// _filter_active TRUE => ALLOW MESSAGE
//_filter_active FALSE => DISABLLOE MESSAGE
void can_id_filter::set_filter(const unsigned short int _id,const bool _allow) {
    LOG_F(INFO, "can_id_filter::set_filter SET FILTER FOR %i to %i", _id, _allow);
    filter_list[_id] = _allow;
}



//RETURN TRUE IF MESSAGE IS ALLOWED
bool can_id_filter::check_message(const unsigned short int _id) {

    if(!can_id_filter::check_key_exists(_id)){
        return invert;
    }


    return filter_list[_id];
}

bool can_id_filter::check_key_exists(const unsigned short int _id){
    if(filter_list.find(_id) != filter_list.end()){
        return true;
    }
    return false;
}

can_id_filter::~can_id_filter() {
    filter_list.clear();
}


