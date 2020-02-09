#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "logging.h"

#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <unordered_map>
#include <memory>

namespace lython{

using Logger = std::shared_ptr<spdlog::logger>;


Logger new_logger(char const* name){
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    auto console = std::make_shared<spdlog::logger>(name, stdout_sink);

    console->set_level(spdlog::level::level_enum::trace);
    console->flush_on(spdlog::level::level_enum::trace);

    spdlog::register_logger(console);
    spdlog::set_pattern("[%L] [%d-%m-%Y %H:%M:%S.%e] [%t] %v");

    return console;
}

Logger root(){
    static Logger log = new_logger("root");
    return log;
}

static constexpr spdlog::level::level_enum log_level_spd[] = {
    spdlog::level::level_enum::trace,
    spdlog::level::level_enum::info,
    spdlog::level::level_enum::warn,
    spdlog::level::level_enum::debug,
    spdlog::level::level_enum::err,
    spdlog::level::level_enum::critical
};


void spdlog_log(LogLevel level, std::string const& msg){
    root()->log(log_level_spd[level], msg);
}

const char* log_level_str[] = {
    "[T] TRACE",
    "[I]  INFO",
    "/!\\  WARN",
    "[D] DEBUG",
    "[E] ERROR",
    "[!] FATAL"
};

std::string format_code_loc(const char*, const char* function, int line){
    return fmt::format(
        "{}:{}", function, line);
}


std::string format_code_loc_trace(const char*, const char* function, int line){
    return fmt::format(
        "{:>25}:{:4}", function, line);
}


// instead of setting a single log level for the entire program allow to cherry pick
// which level is enabled
std::unordered_map<LogLevel, bool>& log_levels(){
    static std::unordered_map<LogLevel, bool> levels{
        {Info,  true},
        {Warn,  true},
        {Debug, true},
        {Error, true},
        {Fatal, true},
        {Trace, true}
    };
    return levels;
}

void set_log_level(LogLevel level, bool enabled){
    log_levels()[level] = enabled;
}

bool is_log_enabled(LogLevel level){
    return log_levels()[level];
}

const char* trace_start = "{} {}+-> {}";
const char* trace_end = "{} {}+-< {}";

}
