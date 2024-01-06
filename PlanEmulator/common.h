#pragma once

#include <vector>
#include <mutex>
#include <future>
#include <variant>
#include <map>

#include <iostream>

#define LOG(x) std::cout << x << endl;

typedef std::map<std::string, std::map<std::string, std::list<std::pair<std::variant<int, double>, std::chrono::system_clock::time_point> >* >* > datastruct4;

struct ControlData {
    std::mutex mx;
    std::condition_variable cv;
    std::atomic<char> state;
    std::vector<unsigned char>* pBuf;
    std::promise<void>* pProm;
};