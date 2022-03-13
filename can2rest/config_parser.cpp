#include "config_parser.h"

//STATIC INIT
config_parser *config_parser::instance = nullptr;
std::mutex config_parser::acces_lock_mutex;


std::string config_parser::toJson() {

    //CREATE JSON ROOT ELEMENT
    json11::Json::object root;
    json11::Json::object settings;
    json11::Json::object settings_user;
    constexpr auto &entries = magic_enum::enum_values<config_parser::CFG_ENTRY>();
    //FOR EACH CONFIG ENTRY ADD ENTRY TO ROOT ELEMENT
    for (size_t i = 0; i < entries.size(); i++) {
        //CAST ENUM KEY TO STRING
        const std::string_view cfg_name = magic_enum::enum_name(entries.at(i));
        //SKIP EMPTY NAMES
        if (std::string(cfg_name).empty()) {
            continue;
        }
        //FINALLY ADD ELEMENT
        settings[std::string(cfg_name)] = config_store[entries.at(i)];


    }


    //SUMP TO JSON
#ifdef INI_SETTINGS_CATEGORY_TOKEN
    root[INI_SETTINGS_CATEGORY_TOKEN] = settings;
#else
    root["SETTINGS"] = settings;
#endif

    std::string json_str = json11::Json(root).dump();

    return json_str;
}

bool config_parser::loadFromJson(json11::Json::object _jsobj, bool load_only_user_data) {
    //NOW READ ENTRIES
    constexpr auto &entries = magic_enum::enum_values<config_parser::CFG_ENTRY>();

    if (!load_only_user_data) {
        //CHECK
#ifdef INI_SETTINGS_CATEGORY_TOKEN
        const std::string SETTINGS_TOKEN = INI_SETTINGS_CATEGORY_TOKEN;
#else
        const std::string SETTINGS_TOKEN = "SETTINGS";
#endif

        if (!_jsobj[SETTINGS_TOKEN].is_null() && _jsobj.find(SETTINGS_TOKEN) != _jsobj.end() &&
            _jsobj[SETTINGS_TOKEN].is_object()) {
            json11::Json::object settings_obj = _jsobj[SETTINGS_TOKEN].object_items();
            //GET SINGLE ITEMS
            for (size_t i = 0; i < entries.size(); i++) {
                const std::string_view cfg_name = magic_enum::enum_name(entries.at(i));
                if (std::string(cfg_name).empty()) {
                    continue;
                }
                //GET OBJECT VALUE STORE IT IN CONFIG_STORE
                if (settings_obj.find(std::string(cfg_name)) != settings_obj.end() &&
                    settings_obj[std::string(cfg_name)].is_string() && !settings_obj[std::string(cfg_name)].is_null()) {
                    config_store[entries.at(i)] = settings_obj[std::string(cfg_name)].string_value();
                }
            }
        }
    }

    cfg_loaded_success = true;
    return cfg_loaded_success;
}

bool config_parser::loadFromJson(const std::string &_jsonstr, bool load_only_user_data) {
    if (_jsonstr.empty()) {
        return false;
    }
    //PARSE JSON STRING
    std::string parse_err = "";
    json11::Json json_root = json11::Json::parse(_jsonstr.c_str(), parse_err);
    json11::Json::object root_obj = json_root.object_items();
    //CHECK PARSE RESULT
    if (!parse_err.empty()) {
        return false;
    }

    return loadFromJson(root_obj, load_only_user_data);
}

config_parser::config_parser() {
    reader = INIReader();
}

config_parser::~config_parser() {
}

config_parser *config_parser::getInstance() {
    std::lock_guard<std::mutex> lck(config_parser::acces_lock_mutex);
    if (config_parser::instance == nullptr) {
        config_parser::instance = new config_parser();
    }
    return config_parser::instance;
}

bool config_parser::loadConfigFile(const std::string &_file) {
    LOG_F(INFO, "config_arser::loadConfigFile WITH FILE %s", _file.c_str());
    //PARSE GIVEN INI FILE
    reader = INIReader(_file);
    if (reader.ParseError() != 0) {
        return false;
    }

    //LOAD VALUES TO MAP
    constexpr auto &entries = magic_enum::enum_values<config_parser::CFG_ENTRY>();

    for (size_t i = 0; i < entries.size(); i++) {
        const std::string_view cfg_name = magic_enum::enum_name(entries.at(i));
        if (std::string(cfg_name).empty()) {
            continue;
        }
#ifdef INI_SETTINGS_CATEGORY_TOKEN
        config_store[entries.at(i)] = reader.Get(INI_SETTINGS_CATEGORY_TOKEN, std::string(cfg_name), "");
#else
        config_store[entries.at(i)] = reader.Get("SETTINGS", std::string(cfg_name), "");
#endif

    }


    cfg_loaded_success = true;
    return true;
}

bool config_parser::writeConfigFile(const std::string &_file) {
    LOG_F(INFO, "config_parser::writeConfigfile save into %s", _file.c_str());
    //OPEN TEXT FILE
    std::ofstream myfile;
    myfile.open(_file.c_str());
    if (!myfile.is_open()) {
        return false;
    }

    //WRITE COMMENTS
#ifdef INIFILE_COMMENTS
    myfile << INIFILE_COMMENTS << "\n";
#endif
    //WRITE INI FILE CATEGORY
#ifdef INI_SETTINGS_CATEGORY_TOKEN
    myfile << "[" << INI_SETTINGS_CATEGORY_TOKEN << "]\n";
#else
    myfile << "[SETTINGS]\n";
#endif
    //FOR EACH ENTRY IN ENUM
    constexpr auto &entries = magic_enum::enum_values<config_parser::CFG_ENTRY>();
    //WRITE NORMAL CONFIG
    for (size_t i = 0; i < entries.size(); i++) {
        const std::string_view cfg_name = magic_enum::enum_name(entries.at(i));
        if (std::string(cfg_name).empty()) {
            continue;
        }

        myfile << std::string(cfg_name) << "=" << config_store[entries.at(i)] << "\n";
    }
    //FINALLY WRITE FILE



    myfile.close();
    return true;
}

void config_parser::loadDefaults() {

    LOG_F(INFO, "config_parser::loadDefaults FOR CONFIG");

    config_store[config_parser::CFG_ENTRY::CAN_SOCKET_IF_NAME] = "can0";
    config_store[config_parser::CFG_ENTRY::CAN_DISABLE_PACKET_FILTER] = "1";


    config_store[config_parser::CFG_ENTRY::REST_WEBSERVER_ENABLE] = "1";
    config_store[config_parser::CFG_ENTRY::REST_WEBSERVER_BIND_PORT] = "4242";
    config_store[config_parser::CFG_ENTRY::REST_WEBSERVER_BIND_HOST] = "0.0.0.0";

    config_store[config_parser::CFG_ENTRY::REDIS_ENABLE] = "1";
    config_store[config_parser::CFG_ENTRY::REDIS_HOST] = "127.0.0.1";
    config_store[config_parser::CFG_ENTRY::REDIS_PORT] = "6379";
    config_store[config_parser::CFG_ENTRY::REDIS_FIELD_PREFIX] = "";
    config_store[config_parser::CFG_ENTRY::REDIS_HASH_NAME] = "CAN";


    config_store[config_parser::CFG_ENTRY::REDIS_ENABLE_WRITE] = "1";


}

void config_parser::set(const config_parser::CFG_ENTRY _entry, const std::string &_value,
                        const std::string &_conf_file_path) {

    LOG_F(INFO, "config_parser::set UPDATE CONFIG ENTRY %s, %s", _value.c_str(),
          std::string(magic_enum::enum_name(_entry)).c_str());
    config_store[_entry] = _value;

    if (!_conf_file_path.empty()) {
        writeConfigFile(_conf_file_path);
    }
}

void
config_parser::setInt(const config_parser::CFG_ENTRY _entry, const int _value, const std::string &_conf_file_path) {
    set(_entry, std::to_string(_value), _conf_file_path);
}

bool config_parser::createConfigFile(const std::string &_file, const bool _load_directly) {
    //WRITE SETTINGS TO FILE
    writeConfigFile(_file);
    //LOAD FILE IF FLAG SET
    if (_load_directly) {
        return loadConfigFile(_file);

    }
    return true;
}

bool config_parser::configLoaded() {
    return cfg_loaded_success;
}

bool config_parser::getInt(const config_parser::CFG_ENTRY _entry, int &_ret) {

    if (!config_parser::getInstance()->get(_entry).empty()) {
        _ret = std::atoi(get(_entry).c_str());
        return true;
    }
    return false;

}

bool config_parser::getBool(const config_parser::CFG_ENTRY _entry, bool &_ret) {

    if (!config_parser::getInstance()->get(_entry).empty()) {

        if (get(_entry) == "true" || get(_entry) == "TRUE" || get(_entry) == "True") {
            _ret = true;
            return true;
        } else if (get(_entry) == "false" || get(_entry) == "FALSE" || get(_entry) == "False") {
            _ret = false;
            return true;
        }


        const int ret = std::atoi(get(_entry).c_str());
        if (ret > 0) {
            _ret = true;
            return true;
        } else {
            _ret = false;
            return true;
        }
    }
    return false;

}

bool config_parser::getBool_nocheck(const config_parser::CFG_ENTRY _entry) {
    bool tmp = false;
    if (!getBool(_entry, tmp)) {
        LOG_F(INFO, "config_parser::getBool_nocheck ERROR READ KEY FAILED%s",
              std::string(magic_enum::enum_name(_entry)).c_str());
        return false;
    }

    return tmp;
}

int config_parser::getInt_nocheck(const config_parser::CFG_ENTRY _entry) {
    int ret = 0;
    if (!getInt(_entry, ret)) {
        LOG_F(INFO, "config_parser::getInt_nocheck ERROR READ KEY FAILED%s",
              std::string(magic_enum::enum_name(_entry)).c_str());
        return 0;
    }
    return ret;
}

std::string config_parser::get(const config_parser::CFG_ENTRY _entry) {
    if (!cfg_loaded_success) {
        return "";
    }

    return config_store[_entry];
}

std::map<std::string, std::string> config_parser::get_raw_config_map() {
    std::map<std::string, std::string> tmp = reader.get_value_map();
    return tmp;
}


