#ifndef REMC_SPDLOG_H_
#define REMC_SPDLOG_H_

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
   const std::string&        logger_name, 
   const std::string&        logfile_path,
   spdlog::level::level_enum log_level = spdlog::level::debug) 
{
   auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      logfile_path, 
      1024 * 1024 * 10, 1
   );
   file_sink->set_level(spdlog::level::info);

   auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
   console_sink->set_level(log_level);

   auto logger = std::make_shared<spdlog::logger>(
      logger_name.data(), 
      spdlog::sinks_init_list{file_sink, console_sink}
   );
   logger->set_pattern("[%Y-%m-%d::%H-%M-%S](%^%l%$) %v");
   logger->set_level(log_level);
   logger->flush_on(spdlog::level::info);

   spdlog::register_logger(logger);
}

template<typename... Args>
inline void Log(
   std::string_view          name, 
   spdlog::level::level_enum level, 
   spdlog::string_view_t     fmt, 
   Args&&...                 args) 
{
   auto logger = spdlog::get(name.data());
   if (logger) {
#if SPDLOG_VER_MAJOR >= 1 && SPDLOG_VER_MINOR >= 11
      logger->log(level, spdlog::fmt_lib::runtime(fmt), std::forward<Args>(args)...);
#else
      // maybe error here
      logger->log(level, fmt, std::forward<Args>(args)...);
#endif
   }
   else std::cerr << std::format("logger <{}> not found!\n", name);
}

template<typename... Args>
inline void GlobalLog(
   spdlog::level::level_enum level, 
   spdlog::string_view_t     fmt,  
   Args&&...                 args) 
{
   static spdlog::logger* logger = [](){
      auto ptr_logger = spdlog::get(DEFAULT_GLOBAL_LOGGER_NAME);
      if (!ptr_logger) {
         InitFileConsoleLogger(DEFAULT_GLOBAL_LOGGER_NAME, 
                               DEFAULT_LOGFILE_PATH);
         ptr_logger = spdlog::get(DEFAULT_GLOBAL_LOGGER_NAME);
      }
      return ptr_logger ? ptr_logger.get() : nullptr;
   }();

   if (logger) [[likely]] {
#if SPDLOG_VER_MAJOR >= 1 && SPDLOG_VER_MINOR >= 11
      logger->log(level, spdlog::fmt_lib::runtime(fmt), std::forward<Args>(args)...);
#else
      // maybe error here
      logger->log(level, fmt, std::forward<Args>(args)...);
#endif
   }
   else std::cerr << "global logger not found or not init!\n";
}

} // namespace remc

#define GlobalLogInfo(...)    remc::GlobalLog(spdlog::level::info,  __VA_ARGS__)

#define GlobalLogWarning(...) remc::GlobalLog(spdlog::level::warn,  __VA_ARGS__)

#define GlobalLogError(...)   remc::GlobalLog(spdlog::level::err,   __VA_ARGS__)

#define GlobalLogDebug(...)   remc::GlobalLog(spdlog::level::debug, __VA_ARGS__)

#endif // REMC_SPDLOG_H_
