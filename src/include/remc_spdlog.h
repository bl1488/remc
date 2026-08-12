#ifndef SPDLOG_WRAPPER_H_
#define SPDLOG_WRAPPER_H_

#include "spdlog/common.h"
#include <iostream>
#include <string_view>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace remc {

constexpr const char* const DEFAULT_GLOBAL_LOGGER_NAME = "global";
constexpr const char* const DEFAULT_LOGFILE_PATH       = "logs/log.txt";

inline void InitFileConsoleLogger(
   std::string_view          logger_name, 
   std::string_view          logfile_path,
   spdlog::level::level_enum log_level = spdlog::level::debug) 
{
   auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      logfile_path.data(), 1024 * 1024 * 10, 1
   );
   file_sink->set_level(spdlog::level::info);

   auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
   console_sink->set_level(log_level);

   auto logger = std::make_shared<spdlog::logger>(
      logger_name.data(), spdlog::sinks_init_list{file_sink, console_sink}
   );
   logger->set_pattern("[%Y-%m-%d::%H-%M-%S](%^%l%$) %v");
   logger->set_level(log_level);
   logger->flush_on(spdlog::level::info);

   spdlog::register_logger(logger);
}

template<typename... Args>
inline void Log(
   std::string_view name, 
   spdlog::level::level_enum level, 
   std::string_view fmt,  
   Args&&... args) 
{
   auto logger = spdlog::get(name.data());
   if (logger)
      // using spdlog runtime string
      logger->log(level, spdlog::fmt_lib::runtime(fmt), std::forward<Args>(args)...);
   else std::cerr << std::format("logger <{}> not found!\n", name);
}

} // namespace remc

#define GlobalLogInfo(...)    remc::Log(remc::DEFAULT_GLOBAL_LOGGER_NAME, spdlog::level::info,   __VA_ARGS__)

#define GlobalLogWarning(...) remc::Log(remc::DEFAULT_GLOBAL_LOGGER_NAME, spdlog::level::warn,   __VA_ARGS__)

#define GlobalLogError(...)   remc::Log(remc::DEFAULT_GLOBAL_LOGGER_NAME, spdlog::level::err,    __VA_ARGS__)

#define GlobalLogDebug(...)   remc::Log(remc::DEFAULT_GLOBAL_LOGGER_NAME, spdlog::level::debug,  __VA_ARGS__)

#endif // SPDLOG_WRAPPER_H_
