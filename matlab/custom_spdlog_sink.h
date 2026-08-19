#pragma once
#include <spdlog/sinks/base_sink.h>
#include "matlab_logger.h"
#include <filesystem>

template<typename Mutex>
class MatlabSink : public spdlog::sinks::base_sink<Mutex> {
public:
    MatlabSink(MatlabLogger& matlab_logger)
        : matlab_logger_(matlab_logger) {}

    static int map_spdlog_level(spdlog::level::level_enum level) {
        switch (level) {
            case spdlog::level::trace:
            case spdlog::level::debug:
                return LEVEL_DEBUG;
            case spdlog::level::info:
                return LEVEL_INFO;
            case spdlog::level::warn:
                return LEVEL_WARN;
            case spdlog::level::err:
            case spdlog::level::critical:
                return LEVEL_ERROR;
            default:
                return LEVEL_INFO;
        }
    }
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        std::string s(msg.payload.data(), msg.payload.size());
        spdlog::level::level_enum level = msg.level;
        std::string filename_clean = std::filesystem::path(msg.source.filename).filename().string();
        int maltab_level = map_spdlog_level(level);
        matlab_logger_.log(maltab_level, s, filename_clean, msg.source.line);
    }
    void flush_() override {}

private:
    MatlabLogger& matlab_logger_;
};