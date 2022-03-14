//
// Created by Marcel Ochsendorf on 25.02.22.
//

#include "redis_interface.h"

redis_interface::redis_interface() = default;

void redis_interface::set_connection_details(const std::string _redis_host, const int _port) {

    if(_redis_host.empty()){
        LOG_F(ERROR, "redis_interface::set_connection_details _redis_host IS EMPTY");
        return;
    }

    if(_port <= 0 || _port > 65536){
        LOG_F(ERROR, "redis_interface::set_connection_details _port IS OUT OF RANGE 1-65536");
        return;
    }

    cfg_lock.lock();
    redis_host = _redis_host;
    redis_port = _port;
    cfg_lock.unlock();
}

void redis_interface::enable_service(const bool _en) {
    if(_en){
        if(thread_running){
            LOG_F(ERROR,"redis_interface::enable_service => THREAD ALREADY RUNNING");
            return;
        }
        std::thread t1(recieve_thread_function, this);
        t1.detach();
        update_thread = &t1;
        thread_running = true;
        LOG_F(INFO,"redis_interface start thread");
    }else{
        if (update_thread)
        {
            thread_running = false;
            update_thread->join();
            LOG_F(INFO,"redis_interface stop thread");
        }
    }
}

void redis_interface::store_frame(const CAN_MESSAGE _frame) {
    if(!enable_write){return;}

    if(frame_json_ptr == nullptr){
        LOG_F(ERROR,"redis_interface::store_frame frame_json_ptr IS NULL");
        return;
    }


    const std::string json = frame_json_ptr->convert_frame(_frame);

    if(!json.empty()){

        write_lock.lock();
        write_map.insert(std::make_pair(redis_field_prefix + std::to_string(_frame.frame.id), json));
        write_lock.unlock();

    }
}

void redis_interface::set_field_prefix(const std::string _prefix) {
    if(_prefix.empty()){
        LOG_F(WARNING, "redis_interface::set_hash_prefix prefox is empty");
    }else{
        LOG_F(INFO, "redis_interface::set_hash_prefix set prefix to %s", _prefix.c_str());
    }

    redis_field_prefix = _prefix;
}


void redis_interface::set_hash_name(const std::string _name){
    if(_name.empty()){
        LOG_F(ERROR, "redis_interface::set_hash_name _name is empty");
        return;
    }else{
        LOG_F(INFO, "redis_interface::set_hash_prefix set prefix to %s", _name.c_str());
    }

    hash_name = _name;
}
void redis_interface::recieve_thread_function(redis_interface *_this) {

    while(_this->thread_running){

        sw::redis::ConnectionOptions redis_con_details;
        _this->cfg_lock.lock();
        const std::string hash_name_tmp = _this->hash_name;
        redis_con_details.host = _this->redis_host;
        redis_con_details.port = _this->redis_port;
        _this->cfg_lock.unlock();
        redis_con_details.socket_timeout = std::chrono::milliseconds(100);

        //CREATE REDIS CONNECTION
        try {
       sw::redis::Redis redis =  sw::redis::Redis(redis_con_details);


       while(_this->thread_running){
            //KEEP LOCKTIME SHORT
            std::unordered_map<std::string, std::string> write_map_tmp;

            //COPY; CLEAR MAP; UNLOCK
            _this->write_lock.lock();
            write_map_tmp = _this->write_map;
            _this->write_map.clear();
            _this->write_lock.unlock();

            if(write_map_tmp.size() > 0){
                //STORE IN SET
                redis.hmset(hash_name_tmp, write_map_tmp.begin(), write_map_tmp.end());
                //PUBLISH UPDATE MESSAGE WITH KEY IN IT
                for (auto& it: write_map_tmp) {
                    redis.publish(hash_name_tmp, ""+std::string(it.first));
                }

                write_map_tmp.clear();
           }

       }




        } catch (const sw::redis::Error &err) {
            LOG_F(ERROR, "REDIS CONNECTION ERROR %s", err.what());
        }
    }

}

void redis_interface::enable_write_to_db(bool _en) {
    enable_write = _en;
}

void redis_interface::set_can_to_json_instance(can_frame_to_json* _frame_json_ptr){
    if(_frame_json_ptr == nullptr){
        LOG_F(ERROR,"redis_interface::set_can_to_json_instance passed _frame_json_ptr is NULLPTR");
    }

    frame_json_ptr = _frame_json_ptr;
}
