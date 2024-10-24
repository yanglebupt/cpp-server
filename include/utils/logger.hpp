#pragma once

#include <iostream>
#include <string>
#include <cstdarg>
#include <ctime>
#include <thread>
#include <mutex>
#include <fstream>
#include <unistd.h>
#include <chrono>
#include <functional>
#include <queue>
#include "./tsqueue.hpp"

#define RESET "\033[0m"
#define RED "\033[31m"     /* Red */
#define GREEN "\033[32m"   /* Green */
#define YELLOW "\033[33m"  /* Yellow */
#define BLUE "\033[34m"    /* Blue */
#define MAGENTA "\033[35m" /* Magenta */
#define CYAN "\033[36m"    /* Cyan */
#define WHITE "\033[37m"   /* White */

// 背景
#define BOLDRED "\033[1m\033[31m"   /* Bold Red */
#define BACKGROUND_GREEN "\033[42m" /* Green Background */

// 可变长参数，支持格式化字符串输入
#define info(format, ...) logger::color_log(WHITE, log_level::info, format, ##__VA_ARGS__);
#define warn(format, ...) logger::color_log(YELLOW, log_level::warn, format, ##__VA_ARGS__);
#define error(format, ...) logger::color_log(RED, log_level::error, format, ##__VA_ARGS__);
#define ok(format, ...) logger::color_log(GREEN, log_level::info, format, ##__VA_ARGS__);
#define color(color, format, ...) logger::color_log(color, log_level::info, format, ##__VA_ARGS__);

enum log_level
{
  info,
  warn,
  error,
  level_count
};

struct logger_config
{
  inline static char *log_level_name[log_level::level_count] = {
      (char *)"Info", (char *)"Warn", (char *)"Error"};
  inline static int timestr_max_length = 45;

  // 是否开启 log
  bool enable = true;
  // 是否打印时间
  bool show_time = true;
  // 是否打印线程 ID
  bool show_thread_id = true;
  // 是否打印堆栈信息
  bool show_trace = false;
  // 是否显示分割符
  bool show_separator = true;
  // 是否保存到文件
  bool enable_save = false;
  // 写文件是否覆盖，不覆盖每次都会新建一个文件保存日志
  bool save_override = true;
  // logger 文件的最大字节数，超过则会新建一个文件进行保存
  int logfile_max_size = 10 * 1024 * 1024;
  // 当 log 很大时，是否拆分成多个小份存到不同的文件
  bool split_log = false;
  // 是否由外部线程/池控制打印和输出日志文件
  bool external_log = true;

  // 默认保存路径在根目录下的 logs 文件夹
  char *save_dir = (char *)"./logs";
  // log 文件名
  char *save_filename = (char *)"ConsoleLog";
  // 打印的分割符
  char *separator = (char *)">>";
  // 打印的时间格式
  char *log_time_format = (char *)"%Y/%m/%d-%H:%M:%S";
  // 日志文件名的时间格式
  char *savefile_time_format = (char *)"%Y-%m-%d@%H-%M-%S";

  // 打印的 level，小于等于该 level 的都将打印，大于的则不打印
  log_level log_level = log_level::error;
  // 日志回调函数
  std::function<void(const char *)> log_to = nullptr;
};

struct logger_task
{
  const char *clr;
  std::queue<std::queue<char *>> lines;
  logger_task(const char *color, std::queue<std::queue<char *>> &&lines) : clr(color), lines(std::move(lines)) {};
  logger_task(const logger_task &task)
  {
    clr = task.clr;
    lines = task.lines;
  };
  logger_task(logger_task &&task)
  {
    clr = task.clr;
    lines = std::move(task.lines);
  };
  logger_task &operator=(const logger_task &task)
  {
    if (this != &task)
    {
      clr = task.clr;
      lines = task.lines;
    }
    return *this;
  };
  logger_task &operator=(logger_task &&task)
  {
    if (this != &task)
    {
      clr = task.clr;
      lines = std::move(task.lines);
    }
    return *this;
  };
};

struct logger
{
  inline static logger_config cfg{};

  // if enable_save is false, will return nullptr
  static const char *current_log_filename()
  {
    return log_filename;
  }

  static void enable_setting()
  {
    create_log_file();
  }

  static void color_log(const char *color, log_level level, const char *format, ...)
  {
    if (!cfg.enable || cfg.log_level < level)
      return;

    // 获取时间
    char *time_str = (char *)"\0";
    if (cfg.show_time)
    {
      int size = 0;
      time_str = format_time(cfg.log_time_format, 1, &size);
      time_str[size] = ' ';
      time_str[size + 1] = '\0';
    }

    // 获取线程 ID
    std::thread::id tid = std::this_thread::get_id();
    char *thread_str = cfg.show_thread_id ? format_str("TID: %d %s", *(unsigned int *)&tid, "") : (char *)"\0";

    // 设置分割符
    char *sep_str = cfg.show_separator ? format_str("%s %s", cfg.separator, "") : (char *)"\0";

    // 打印前缀
    char *prefix_str = format_str("[%s] %s%s%s", logger_config::log_level_name[level], time_str, thread_str, sep_str);

    // 可以先释放
    if (cfg.show_separator)
      delete[] sep_str;
    if (cfg.show_thread_id)
      delete[] thread_str;
    if (cfg.show_time)
      delete[] time_str;

    // 格式化字符串
    va_list args;
    va_start(args, format);
    char *message_str = format_str(format, args);
    va_end(args);

    // 打印调用信息
    char *stack_str = cfg.show_trace ? format_str("\t %s %s line %d", __FILE__, __PRETTY_FUNCTION__, __LINE__) : nullptr;

    // 输出
    std::queue<std::queue<char *>> lines{};
    {
      std::queue<char *> line{};
      line.push(prefix_str);
      line.push(message_str);
      lines.push(std::move(line));
    }
    if (cfg.show_trace)
    {
      std::queue<char *> line{};
      line.emplace(stack_str);
      lines.push(std::move(line));
    }

    std::unique_lock<std::mutex> lock(fw_mutex);

    if (cfg.external_log)
      logger_tasks.emplace_back(logger_task(color, std::move(lines)));
    else
      output_lines(color, lines);
  }

  static void ex_log()
  {
    while (true)
    {
      logger_tasks.wait();
      if (logger_tasks.exited)
      {
        while (logger_tasks.size() > 0)
        {
          logger_task lt = logger_tasks.pop_front();
          output_lines(lt.clr, lt.lines);
        }
        break;
      }
      logger_task lt = logger_tasks.pop_front();
      output_lines(lt.clr, lt.lines);
    }
  }

  static void terminate()
  {
    logger_tasks.try_exit();
  }

private:
  inline static std::ofstream fw = nullptr;
  inline static char *log_filename = nullptr;
  inline static std::mutex fw_mutex = std::mutex();
  inline static tsqueue<logger_task> logger_tasks{};

  // 输出
  static void output_lines(const char *color, std::queue<std::queue<char *>> &lines)
  {
    std::cout << color;

    // 确保一次打印，不会拆分到多个文件中，需要在 output_to 之前检测文件大小并新建
    if (!cfg.split_log && cfg.enable_save && log_filename != nullptr && fw.tellp() >= cfg.logfile_max_size)
      create_log_file();

    while (lines.size() > 0)
    {
      std::queue<char *> &line = lines.front();

      while (line.size() > 0)
      {
        output_to(line.front());
        line.pop();
      }
      output_to("\n");
      lines.pop();
    }

    std::cout << RESET;
  }

  // 创建输出文件流
  static char *create_log_file()
  {
    // 先关闭之前的日志
    if (log_filename != nullptr)
      fw.close();
    if (cfg.enable_save)
    {
      // 文件夹不存在，无法新建文件
      if (access(cfg.save_dir, F_OK) == -1)
        if (mkdir(cfg.save_dir) == -1)
          error("mkdir %s failed", cfg.save_dir);
      if (cfg.save_override)
      {
        // 必须要写入 << 然后及时 close/flush ，才会出现新建的文件
        log_filename = format_str("%s/%s.txt", cfg.save_dir, cfg.save_filename);
        fw = std::ofstream(log_filename, std::ios::trunc);
      }
      else
      {
        char *time_str = format_time(cfg.savefile_time_format);
        log_filename = format_str("%s/%s@%s.txt", cfg.save_dir, time_str, cfg.save_filename);
        delete[] time_str;
        fw = std::ofstream(log_filename, std::ios::app);
      }
    }
    else
    {
      log_filename = nullptr;
      fw = nullptr;
    }
    return log_filename;
  }

  // 需要精确到 ms 格式化时间
  static char *format_time(const char *format, int extra_size = 0, int *out_size = nullptr)
  {
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    time_t ticks = std::chrono::system_clock::to_time_t(tp);

    std::string ms = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count() % 1000);

    tm *ptm = localtime(&ticks);
    int max_size = logger_config::timestr_max_length;
    char tmp[max_size];
    // strftime 不支持 NULL 0 来获取 buffer 大小，并且 format 不支持毫秒
    int size = strftime(tmp, max_size, format, ptm) + 4;
    int t_size = size + 1 + extra_size;
    char *str = new char[t_size];
    strftime(str, t_size, format, ptm);
    str[size - 4] = '.';
    str[size - 3] = ms[0];
    str[size - 2] = ms[1];
    str[size - 1] = ms[2];
    str[size] = '\0';
    if (out_size != nullptr)
      *out_size = size;
    return str;
  }

  // 格式化字符串
  static char *format_str(const char *format, va_list args)
  {
    int size = vsnprintf(NULL, 0, format, args);
    char *str = new char[size + 1];
    vsnprintf(str, size + 1, format, args);
    str[size] = '\0';
    return str;
  }

  // 格式化字符串
  static char *format_str(const char *format, ...)
  {
    va_list args;
    va_start(args, format);

    int size = vsnprintf(NULL, 0, format, args);
    char *str = new char[size + 1];
    vsnprintf(str, size + 1, format, args);
    str[size] = '\0';

    va_end(args);
    return str;
  }

  // 输出打印内容
  static void output_to() {}

  // 输出打印内容
  template <typename T, typename... Args>
  static void output_to(T *message, Args... args)
  {
    std::cout << message;

    if (cfg.log_to != nullptr)
      cfg.log_to(message);

    if (cfg.enable_save && log_filename != nullptr)
    {
      if (cfg.split_log && fw.tellp() >= cfg.logfile_max_size)
        create_log_file();

      fw << message;
      fw.flush();
    }

    // 释放
    if constexpr (!std::is_const_v<T>)
      delete[] message;

    output_to(args...);
  }
};
