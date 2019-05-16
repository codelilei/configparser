// Notes:  Config parser
// Author: Andrew Lee
// Date:   2019-5-14
// Email:  code.lilei@gmail.com

#include <iostream>
#include <vector>
#include <sstream>
#include "config_parser.hpp"


template <typename vector_type>
void PrintVector(const vector_type& vec, const char* delim = ",") {
    stringstream ss;
    bool start = true;
    for (auto i : vec) {
        if (start)  start = false;
        else ss << delim;
        ss << i;
    }
    std::cout << ss.str() << endl;
}


void TestCfgParser() {
    // parse a configuration without section
    std::cout << "======parser0======" << std::endl;
    ConfigParser parser0("test/cfg_test0.txt");
    std::cout << "get_a=" << parser0.get("a", 2).to_int() << std::endl;
    std::cout << "get_b=" << parser0.get("b", "false").to_bool() << std::endl;
    std::cout << "get_c=" << parser0.get("c", "0.0").to_float() << std::endl;
    std::cout << "get_d=" << parser0.get("d", "None") << std::endl;
    std::cout << "get_e=";
    auto vec = parser0.get("e", "").to_vector<int>();
    PrintVector(vec);
    // if explicit type convert is not preferred, use get2 interface instead
    int a;
    parser0.get2("a", a, 2);
    std::cout << "get2_a=" << a << std::endl;
    vector<int> vi;
    parser0.get2("e", vi, "");
    std::cout << "get2_e=";
    PrintVector(vi);
    std::cout << "parser0.print()=" << std::endl;
    parser0.print();

    // parse a section-style configuration
    std::cout << "======parser1======" << std::endl;
    ConfigParser parser1("test/cfg_test1.txt");
    parser1.set("b", "true", "abc");
    std::cout << "get_a=" << parser1.get("a", 2, "abc").to_int() << std::endl;
    std::cout << "get_b=" << parser1.get("b", false, "abc").to_bool() << std::endl;
    std::cout << "get_c=" << parser1.get("c", 0.0, "abc").to_float() << std::endl;
    std::cout << "get_d=" << parser1.get("d", "None", "abc") << std::endl;
    parser1.remove("e", "de");
    vector<int> v = { 4, 5, 6 };
    parser1.set("v", v, "de");
    std::cout << "parser1.print()=" << std::endl;
    parser1.print();
    parser1.write("test/cfg_save.txt");

    // create and save a configuration
    std::cout << "======parser2======" << std::endl;
    ConfigParser parser2;
    parser2.set("o21", "str21", "sec1");
    parser2.set("o22", true, "sec1");
    parser2.set("o23", 23, "sec2");
    parser2.set("o24", 24.5, "sec2");
    std::cout << "parser2.print()=" << std::endl;
    parser2.print();
    parser2.write("test/cfg_test2.txt");
}


int main(int argc, char* argv[]) {
   TestCfgParser();
   
   return 0;
}

