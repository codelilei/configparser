// Notes:  Config parser
// Author: Andrew Lee
// Date:   2019-5-14
// Email:  code.lilei@gmail.com

#pragma once
#ifndef CONFIG_PARSER_HPP_
#define CONFIG_PARSER_HPP_

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>

namespace {
using std::string;
using std::stringstream;
using std::vector;
using std::map;
using std::find;


struct ConfigItem {
    string option_;
    string value_;
    // string comment_;

    ConfigItem() : option_(""), value_("") {}
    ConfigItem(const string& option, const string& value) :
        option_(option), value_(value) {}

    bool operator==(const string& rhs) { return this->option_ == rhs; }
};


inline string& lstrip(string& s, const char* c = " \t\n\r\f\v") {
    s.erase(0, s.find_first_not_of(c));
    return s;
}

inline string& rstrip(string& s, const char* c = " \t\n\r\f\v") {
    s.erase(s.find_last_not_of(c) + 1);
    return s;
}

template <class input_type, class output_type>
output_type TypeConvert(const input_type& input) {
    stringstream ss;
    ss << input;
    output_type result;
    ss >> std::boolalpha;  // ss.setf(ios_base::boolalpha);
    ss >> result;
    return result;
}

template <typename T, typename A>
stringstream& operator<<(stringstream& ss, const std::vector<T, A>& vec) {
    bool start = true;
    for (auto i : vec) {
        if (start)  start = false;
        else ss << ",";
        ss << i;
    }
    return ss;
}

template <typename T, typename A>
stringstream& operator>>(stringstream& ss, std::vector<T, A>& vec) {
    static const string delim = ",";
    string::size_type pos(0), pos_pre(0);
    string str = ss.str() + delim;
    while ((pos = str.find(delim, pos)) != string::npos) {
        string s = str.substr(pos_pre, pos - pos_pre);
        vec.push_back(TypeConvert<string, std::vector<T, A>::value_type>(s));
        pos += delim.length();
        pos_pre = pos;
    }
    return ss;
}

// return a mixed type to make type convert convenient 
class MixType {
public:
    MixType() : str_("") {}
    MixType(const string& str) : str_(str) {}
    ~MixType() {}

    void set(const string& str) { str_ = str; }
    void reset() { str_ = ""; }

    string to_string() const { return str_; }
    int to_int() const { return TypeConvert<string, int>(str_); }
    float to_float() const { return TypeConvert<string, float>(str_); }
    bool to_bool() const { return TypeConvert<string, bool>(str_); }

    template <class output_type>
    vector<output_type> to_vector() { return TypeConvert<string, vector<output_type>>(str_); }
    // auto to_vector(const string delim = ",") const ->vector<output_type> {}
    // vector<output_type> to_vector() const {}

private:
    string str_;
};

std::ostream& operator<<(std::ostream& lhs, const MixType& mix_t) {
    lhs << mix_t.to_string();
    return lhs;
}


class ConfigParser {
public:
    ConfigParser() {}
    ConfigParser(const string& cfg_file) { read(cfg_file); }
    ~ConfigParser() { reset(); }

    template <class defval_type>
    MixType& get(const string& option, const defval_type& def_val, string section = default_sec_);

    template <class output_type, class defval_type>
    void get2(const string& option, output_type& dst_var, const defval_type& def_val, string section = default_sec_);

    template <class input_type>
    void set(const string& option, const input_type& value, string section = default_sec_);

    void remove(const string& option, string section = default_sec_);
    void read(const string& cfg_file);
    void write(const string& file_name);
    void print();
    void reset();

private:
    struct FindInfo {
        map<string, vector<ConfigItem>>::iterator sec_iter;
        vector<ConfigItem>::iterator ov_iter;
        bool sec_found;
        bool ov_found;
    } find_info_;

    FindInfo& Find(const string& option, string section);
    string CfgToSting();

    static const string default_sec_;
    MixType mix_t_;
    map<string, vector<ConfigItem>> cfg_info_;
};
const string ConfigParser::default_sec_ = "default_";


template <class defval_type>
inline MixType& ConfigParser::get(const string& option, const defval_type& def_val, string section) {
    auto found = Find(option, section);
    if (found.sec_found && found.ov_found) {
        mix_t_.set(found.ov_iter->value_);
        return mix_t_;
    }
    string def_val_str = TypeConvert<defval_type, string>(def_val);
    mix_t_.set(def_val_str);
    return mix_t_;
}

template <class output_type, class defval_type>
void ConfigParser::get2(const string& option, output_type& dst_var, const defval_type& def_val, string section) {
    auto found = Find(option, section);
    if (found.sec_found && found.ov_found) dst_var = TypeConvert<string, output_type>(found.ov_iter->value_);
    else dst_var = TypeConvert<defval_type, output_type>(def_val);
}

template <class input_type>
inline void ConfigParser::set(const string& option, const input_type& value, string section) {
    string value_str = TypeConvert<input_type, string>(value);
    auto found = Find(option, section);
    if (found.sec_found) {
        auto& ovs = found.sec_iter->second;
        if (found.ov_found) found.ov_iter->value_ = value_str;
        else ovs.push_back(ConfigItem(option, value_str));
    }
    else {
        vector<ConfigItem> cfg_items;
        cfg_items.push_back(ConfigItem(option, value_str));
        cfg_info_.insert(std::make_pair(section, cfg_items));
    }
}

inline void ConfigParser::remove(const string& option, string section) {
    auto found = Find(option, section);
    if (found.sec_found && found.ov_found) {
        auto& ovs = found.sec_iter->second;
        ovs.erase(found.ov_iter);
        // delete a section if no config item exists 
        if (ovs.size() < 1) cfg_info_.erase(found.sec_iter);
    }
}

inline void ConfigParser::read(const string& cfg_file) {
    std::ifstream ifs(cfg_file);
    if (!ifs) {
        std::cerr << "error opening config file: " << cfg_file << std::endl;
        return;
    }

    cfg_info_.clear();
    string line, section, option, value;
    int cnt = 0;
    while (std::getline(ifs, line)) {
        lstrip(line);
        rstrip(line);
        if (line.length() < 1 || line[0] == '#') continue;
        ++cnt;
        auto lbr_pos = line.find('[');
        auto rbr_pos = line.find(']');
        // set section to "default_" if not specified at the beginning
        if ((cnt == 1) && (lbr_pos == string::npos || rbr_pos == string::npos)) {
            section = default_sec_;
            vector<ConfigItem> cfg_items;
            cfg_info_.insert(std::make_pair(section, cfg_items));
        }
        else if (lbr_pos != string::npos && rbr_pos != string::npos) {
            string section_tmp = line.substr(lbr_pos + 1, rbr_pos - lbr_pos - 1);
            lstrip(section_tmp);
            rstrip(section_tmp);
            if (section_tmp.length())  section = section_tmp;
            vector<ConfigItem> cfg_items;
            cfg_info_.insert(std::make_pair(section, cfg_items));
            continue;
        }

        auto equ_pos = line.find('=');
        if (line.find('=') != string::npos) {
            option = line.substr(0, equ_pos);
            rstrip(option);
            if (option.length() < 1)  continue;
            value = line.substr(equ_pos + 1, line.length() - equ_pos - 1);
            lstrip(value);
            cfg_info_[section].push_back(ConfigItem(option, value));
        }
    }

    ifs.close();
}

inline void ConfigParser::write(const string& file_name) {
    std::ofstream ofs(file_name);
    if (!ofs) {
        std::cerr << "error writing config file: " << file_name << std::endl;
        return;
    }
    ofs << CfgToSting();
    ofs.close();
}

inline void ConfigParser::print() { std::cout << CfgToSting(); }

inline void ConfigParser::reset() { cfg_info_.clear(); }

inline ConfigParser::FindInfo& ConfigParser::Find(const string& option, string section) {
    auto sec_found = cfg_info_.find(section);
    if (sec_found != cfg_info_.end()) {
        find_info_.sec_found = true;
        find_info_.sec_iter = sec_found;
        auto& ovs = sec_found->second;
        auto ov_iter = find(ovs.begin(), ovs.end(), option);
        find_info_.ov_found = (ov_iter != ovs.end());
        find_info_.ov_iter = ov_iter;
    }
    else {
        find_info_.sec_found = false;
    }
    return find_info_;
}

inline string ConfigParser::CfgToSting() {
    stringstream ss;
    for (auto& m : cfg_info_) {
        ss << "[" << m.first << "]" << std::endl;
        for (auto& ov : m.second) {
            ss << ov.option_ << "=" << ov.value_ << std::endl;
        }
        ss << std::endl;
    }
    return ss.str();
}

}  // namespace

#endif  // CONFIG_PARSER_HPP_
