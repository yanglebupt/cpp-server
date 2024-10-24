#include <iostream>
#include <utils/logger.hpp>

int main()
{
  logger::cfg.show_time = true;
  logger::cfg.show_thread_id = true;
  logger::cfg.show_separator = true;
  logger::cfg.show_trace = true;
  logger::cfg.enable_save = true;
  logger::cfg.save_override = false;
  logger::cfg.external_log = true;
  std::thread log(&logger::ex_log);

  logger::enable_setting();

  std::cout << logger::current_log_filename() << std::endl;

  std::thread t([]()
                { error("Mike %s %d", "Jack", 32); });

  error("Mike %s %d", "Jack", 32);
  info("Jack");

  ok("Mike %s %d", "Jack", 32);
  color(MAGENTA, "Jack");

  logger::terminate();
  if (log.joinable())
    log.join();
  if (t.joinable())
    t.join();

  std::cout << logger::current_log_filename() << std::endl;

  return 0;
}
