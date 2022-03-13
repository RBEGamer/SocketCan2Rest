//
// Created by Marcel Ochsendorf on 21.02.22.
//

#include "rest_api.h"

rest_api::rest_api(can_frame_to_json *_can_to_json_instance, can_hashtable *_can_hashtable_instance, frame_send_queue* _queue) {
    if(_can_to_json_instance == nullptr || _can_hashtable_instance == nullptr || _queue == nullptr){
        LOG_F(ERROR, "rest_api::rest_api _can_to_json_instance or _can_hashtable_instance or _queue is null");
    }

    can_to_json_instance = _can_to_json_instance;
    can_hashtable_instance = _can_hashtable_instance;
    queue = _queue;
}


void rest_api::set_bind_settings(const std::string _bind_host, const int _port){
    if(_port <= 0 || _port > 65536){
        LOG_F(ERROR, "rest_api::set_bind_settings port invalid %i", _port);
        port = 4242;
    }else{
        port = _port;
    }

    if(_bind_host.empty()){
        LOG_F(ERROR, "rest_api::set_bind_settings _bind_host is empty");
        host = "127.0.0.0";
    }else{
        host = _bind_host;
    }

}

void rest_api::enable_service(bool _en) {
    if(_en){
        if(thread_running){
            LOG_F(ERROR,"rest_api::enable_service => THREAD ALREADY RUNNING");
            return;
        }
        std::thread t1(recieve_thread_function, this);
        t1.detach();
        update_thread = &t1;
        thread_running = true;

        LOG_F(INFO,"rest_api start thread");
    }else{
        if (update_thread)
        {
            thread_running = false;
            update_thread->join();
            LOG_F(INFO,"rest_api stop thread");
        }
    }
}

void rest_api::recieve_thread_function(rest_api *_this) {
    using namespace httplib;
    httplib::Server svr;
    //REGISTER WEBSERVER EVENTS

    enum CONVERSATION_TYPE{
        NONE = 0,
        RAW = 1,
        INT = 2,
        BYTE = 3,
    };




    svr.Get("/", [_this](const Request& req, Response& res) {
        res.status = 200;
        const json11::Json t = json11::Json::object{
            {"version", VERSION},
            {"version", "/version"},
            {"list", "/list"},
            {"send", "/send"},
            {"receive", "/receive"},
            {"types", "/types"},
            {"config", "/config"}
        };

        res.set_content(t.dump(), "application/json");
    });

    svr.Get("/types", [_this](const Request& req, Response& res) {
        res.status = 200;
        const json11::Json t = json11::Json::object{
                {"NONE", "0"},
                {"RAW", "1"},
                {"INT", "2"},
                {"BYTE", "3"}
        };
        res.set_content(t.dump(), "application/json");
    });

    svr.Get("/version", [_this](const Request& req, Response& res) {
        res.status = 200;
        const json11::Json t = json11::Json::object{{"version", VERSION}};
        res.set_content(t.dump(), "application/json");
    });

    svr.Get("/config", [_this](const Request& req, Response& res) {
        res.status = 200;

        res.set_content(config_parser::getInstance()->toJson(), "application/json");
    });



    svr.Post("/send", [_this](const Request& req, Response& res) {
        //TODO CHECK JSON TYPE
        //CAST

        //CHECK BODY CONTENT
        if(req.body.empty()){
            res.status = 500;
            const json11::Json t = json11::Json::object{{"error", "post body empty"}};
            res.set_content(t.dump(), "application/json");
            return;
        }

        //PARSE TO JSON
        std::string parse_err = "";
        json11::Json json_root = json11::Json::parse(req.body.c_str(), parse_err);
        json11::Json::object root_obj = json_root.object_items();

        if(!parse_err.empty()){
            res.status = 500;
            const json11::Json t = json11::Json::object{{"error", "json parse failed"}};
            res.set_content(t.dump(), "application/json");
            return;
        }

        if(root_obj.find("id") != root_obj.end() && !root_obj["id"].is_null() && root_obj["id"].is_number() && root_obj.find("payload") != root_obj.end() && !root_obj["payload"].is_null()) {
            //
            const unsigned int id = root_obj["id"].number_value();
            //CREATE CAN FRAME DEPENDING OF JSON TYPE
            if(root_obj["payload"].is_number()) {
                _this->queue->enqueue_int16(id,root_obj["payload"].int_value());
            }else if(root_obj["payload"].is_string()) {
                _this->queue->enqueue_bytes(id,root_obj["payload"].string_value());
            }else if(root_obj["payload"].is_bool()) {
                _this->queue->enqueue_bool(id,root_obj["payload"].bool_value());
            }else if(root_obj["payload"].is_array()) {

                //ONLY SUPPORT JSON ARRAYS WITH BOOL INTEGER FOR STRING (FIRST CHARAKTER)
                std::vector<unsigned char> tmp_vec;
                for( int i = 0; i < root_obj["payload"].array_items().size(); i++){
                    if(root_obj["payload"].array_items().at(i).is_bool()){
                        tmp_vec.push_back(static_cast<const unsigned char>(root_obj["payload"].array_items().at(i).bool_value()));
                    }else if(root_obj["payload"].array_items().at(i).is_number()){
                        tmp_vec.push_back(static_cast<const unsigned char>(root_obj["payload"].array_items().at(i).bool_value()));
                        //ONLY USE THE FIRST CHAR OF STRING
                    }else if(root_obj["payload"].array_items().at(i).is_string()){
                        if(root_obj["payload"].array_items().at(i).string_value().length() > 0){
                            tmp_vec.push_back(static_cast<const unsigned char>(root_obj["payload"].array_items().at(i).string_value().at(0)));
                        }else{
                            tmp_vec.push_back(0);
                        }
                    }
                }


                _this->queue->enqueue_array(id,tmp_vec);
            }
        }else{
            res.status = 500;
            const json11::Json t = json11::Json::object{{"error", "no json field: id or payload"}};
            res.set_content(t.dump(), "application/json");
            return;
        }



        const json11::Json t = json11::Json::object{
            {"send_status", "ok"}
        };
        res.set_content(t.dump(), "application/json");
    });



    //GET A SPECIFIC CAN MESSAGE
    //
    svr.Get(R"(/receive/(\d+))", [_this](const Request& req, Response& res) {
        auto id = req.matches[1];
        //IF MATCH FOUND FOR /receive/<CAN_FRAME_ID>
        if(id.matched){
            res.status = 200;
            const int conv_id = std::atoi(id.str().c_str());
            //GET MESSAGE FROM STORAGE
            const CAN_MESSAGE msg = _this->can_hashtable_instance->get(conv_id);
            //CHECK IF IT IS VALID => SO IN STORAGE
            if(msg.valid){
                const std::string tmp_res = _this->can_to_json_instance->convert_frame(msg);
                res.set_content(tmp_res, "application/json");
            }else{
                const json11::Json t = json11::Json::object{{"valid", false}};
                res.set_content(t.dump(), "application/json");
            }
        }else{
            res.status = 500;
            const json11::Json t = json11::Json::object{
                {"error", "match error"},
                {"match_first", id.str()}};
            res.set_content(t.dump(), "application/json");
        }
    });


    //LIST ALL CAN IDs
    svr.Get("/list", [_this](const Request& req, Response& res) {
        const json11::Json t = json11::Json::object{
            {"can_frame_ids", _this->can_hashtable_instance->get_id_list()}
        };
        res.set_content(t.dump(), "application/json");
    });



    svr.set_error_handler([](const auto& req, auto& res) {
        res.status = 404;
        const json11::Json t = json11::Json::object{{"error", res.status}};
        res.set_content(t.dump(), "application/json");

    });



    svr.set_exception_handler([](const auto& req, auto& res, std::exception &e) {
        res.status = 500;
        const json11::Json t = json11::Json::object{{"exception", res.status}};
        res.set_content(t.dump(), "application/json");
    });

    //START WEBSERVER
    if(!svr.listen(_this->host.c_str(), _this->port)){
        LOG_F(ERROR,"WEBSERVER BIND FAILED");
        _this->enable_service(false); //STOP SERVER
        return;
    }
    while (_this->thread_running) {

    }
    svr.stop();
}
