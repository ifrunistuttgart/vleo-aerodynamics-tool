#pragma once
#include <string>
#include <vector>
#include <memory>
#include <mexAdapter.hpp>

#define LEVEL_ERROR 3
#define LEVEL_WARN 2
#define LEVEL_INFO 1
#define LEVEL_DEBUG 0
#define LOG(level, msg) do { if ((level) >= LOG_LEVEL) { log((level), std::string(msg), __LINE__); } } while (0)

class MatlabLogger {
public:
    MatlabLogger(std::shared_ptr<matlab::engine::MATLABEngine> matlab_ptr){
        matlab_ptr_ = std::move(matlab_ptr);
    }
    void log(int level, const std::string& msg, std::string file, int line) {
        switch (level) {
            case LEVEL_INFO: info(msg, file, line); break;
            case LEVEL_WARN: warning(msg,file, line); break;
            case LEVEL_ERROR: error(msg, file, line); break;
            case LEVEL_DEBUG: debug(msg, file, line); break;
            default: warning("Unknown log level: " + std::to_string(level), "matalb_logger.h", __LINE__); break;
        }
    }

private:
    void info(const std::string& msg, const std::string& file, int line) {
        std::string text = "[Info] [" + file + ":" + std::to_string(line) + "] " + msg + "\n";
        matlab_ptr_->feval(u"fprintf", 0, std::vector<matlab::data::Array>({
            factory_.createScalar("%s"),
            factory_.createScalar(text)
        }));
    }

    void warning(const std::string& msg, const std::string& file, int line) {
        std::string text = "[Warn] [" + file + ":" + std::to_string(line) + "] " + msg + "\n";
        matlab_ptr_->feval(u"fprintf", 0, std::vector<matlab::data::Array>({
            factory_.createScalar("%s"),
            factory_.createScalar(text)
        }));
    }

    void error(const std::string& msg, const std::string& file, int line) {
        // Outputs to stderr (file ID 2), rendering in bright RED
        std::string text = "[Error] [" + file + ":" + std::to_string(line) + "] " + msg + "\n";

        matlab_ptr_->feval(u"fprintf", 0, std::vector<matlab::data::Array>({
            factory_.createScalar(2),      // File ID: 2 (stderr)
            factory_.createScalar("%s"),   // Format specifier
            factory_.createScalar(text)    // Argument string
        }));
    }

    void debug(const std::string& msg, const std::string& file, int line) {
        std::string text = "[Debug] [" + file + ":" + std::to_string(line) + "] " + msg + "\n";

        matlab_ptr_->feval(u"fprintf", 0, std::vector<matlab::data::Array>({
            factory_.createScalar("%s"),
            factory_.createScalar(text)
        }));
    }

    std::shared_ptr<matlab::engine::MATLABEngine> matlab_ptr_;
    matlab::data::ArrayFactory factory_;
};