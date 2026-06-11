#pragma once
#include <format>
#include <iostream>
#include <string_view>
#include <utility>

#ifdef __ANDROID__
#include <android/log.h>
#endif

struct app_logger {
  enum class LEVEL {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
  };

  static void init(LEVEL level) { current_level = level; }

  template <typename... Args>
  static void dbug(std::string_view fmt, Args&&... args) {
    log(LEVEL::DEBUG, fmt, std::forward<Args>(args)...);
  }
  template <typename... Args>
  static void info(std::string_view fmt, Args&&... args) {
    log(LEVEL::INFO, fmt, std::forward<Args>(args)...);
  }
  template <typename... Args>
  static void warn(std::string_view fmt, Args&&... args) {
    log(LEVEL::WARN, fmt, std::forward<Args>(args)...);
  }
  template <typename... Args>
  static void err(std::string_view fmt, Args&&... args) {
    log(LEVEL::ERROR, fmt, std::forward<Args>(args)...);
  }
  template <typename... Args>
  static void ftal(std::string_view fmt, Args&&... args) {
    log(LEVEL::FATAL, fmt, std::forward<Args>(args)...);
  }

  static inline LEVEL current_level = LEVEL::DEBUG;

  template <typename... Args>
  static void log(LEVEL level, std::string_view fmt, Args&&... args) {
    if (level < current_level)
      return;

    // Efficiently format the string
    std::string formatted_msg = std::vformat(fmt, std::make_format_args(args...));

#ifdef __ANDROID__
    int priority = ANDROID_LOG_DEBUG;
    switch (level) {
      case LEVEL::INFO:
        priority = ANDROID_LOG_INFO;
        break;
      case LEVEL::WARN:
        priority = ANDROID_LOG_WARN;
        break;
      case LEVEL::ERROR:
        priority = ANDROID_LOG_ERROR;
        break;
      case LEVEL::FATAL:
        priority = ANDROID_LOG_FATAL;
        break;
      default:
        priority = ANDROID_LOG_DEBUG;
        break;
    }
    __android_log_print(priority, APP_NAME, "%s", formatted_msg.c_str());
#else
    std::cout << formatted_msg << std::endl;
#endif
  }
};

#define LOGD_LABEL "\033[32mDEBUG\033[0m"
#define LOGI_LABEL "\033[34mINFO\033[0m"
#define LOGW_LABEL "\033[33mWARNING\033[0m"
#define LOGE_LABEL "\033[31mERROR\033[0m"

#define LOGD(fmt, ...)                                                                     \
  app_logger::log(app_logger::LEVEL::DEBUG, "{} [{}:{}] {} " fmt, __TIME__, __FILE_NAME__, \
                  __LINE__, LOGD_LABEL, ##__VA_ARGS__)
#define LOGI(fmt, ...)                                                                    \
  app_logger::log(app_logger::LEVEL::INFO, "{} [{}:{}] {} " fmt, __TIME__, __FILE_NAME__, \
                  __LINE__, LOGI_LABEL, ##__VA_ARGS__)
#define LOGW(fmt, ...)                                                                    \
  app_logger::log(app_logger::LEVEL::WARN, "{} [{}:{}] {} " fmt, __TIME__, __FILE_NAME__, \
                  __LINE__, LOGW_LABEL, ##__VA_ARGS__)
#define LOGE(fmt, ...)                                                                     \
  app_logger::log(app_logger::LEVEL::ERROR, "{} [{}:{}] {} " fmt, __TIME__, __FILE_NAME__, \
                  __LINE__, LOGE_LABEL, ##__VA_ARGS__)
