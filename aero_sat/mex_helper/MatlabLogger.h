//
// Created by Jan_L on 08.06.2026.
//
#pragma once

#include "mex.hpp"
#include <string>
#include <memory>

#define LEVEL_ERROR 2
#define LEVEL_WARN 1
#define LEVEL_INFO 0
#define LOG_LEVEL LEVEL_ERROR
#define LOG(level, msg) do{ if(level >= LOG_LEVEL) { log(level, std::string(msg), __LINE__); } }while(0)

class MatlabLogger: public matlab::mex::Function {
public:
    MatlabLogger();
    ~MatlabLogger() = default;
    void log(int level, const std::string &msg, const int line);
private:
    void info(const std::string &msg, const int line);
    void warning(const std::string &msg, const int line);
    void error(const std::string &msg, const int line);
    std::shared_ptr<matlab::engine::MATLABEngine> matlab_ptr;
    matlab::data::ArrayFactory factory_;
};


