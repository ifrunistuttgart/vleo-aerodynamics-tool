//
// Created by Jan_L on 08.06.2026.
//
#include "MatlabLogger.h"
#include <vector>

MatlabLogger::MatlabLogger() {
    matlab_ptr = getEngine();
}

void MatlabLogger::log(const int level, const std::string &msg, const int line) {
    switch (level) {
        case LEVEL_INFO:
            info(msg, line);
            break;
        case LEVEL_WARN:
            warning(msg, line);
            break;
        case LEVEL_ERROR:
            error(msg, line);
            break;
        default:
            warning("Unknown log level: " + std::to_string(level), __LINE__);
            break;
    }
}

void MatlabLogger::info(const std::string &msg, const int line) {
    matlab_ptr->feval(u"disp", 0, std::vector<matlab::data::Array>({ factory_.createScalar("[Info] [Line: " + std::to_string(line) + "] " + msg) }));
}

void MatlabLogger::warning(const std::string &msg, const int line) {
    matlab_ptr->feval(u"disp", 0, std::vector<matlab::data::Array>({ factory_.createScalar("[Warn] [Line: " + std::to_string(line) + "] " + msg) }));
}

void MatlabLogger::error(const std::string &msg, const int line) {
    matlab_ptr->feval(u"disp", 0, std::vector<matlab::data::Array>({ factory_.createScalar("[Error] [Line: " + std::to_string(line) + "] " + msg) }));
}


