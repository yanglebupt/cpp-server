#pragma once

#include "boost/asio.hpp"

namespace asio = boost::asio;

namespace net::nhttp
{
  struct timer
  {
  private:
    bool is_running = true;
    asio::steady_timer __t;

  public:
    timer(asio::io_context &ctx, std::function<void(void)> _Callback, int ms, bool is_interval) : __t(ctx)
    {
      is_running = true;
      start(_Callback, ms, is_interval);
    }

    void stop()
    {
      is_running = false;
    }

    void start(std::function<void(void)> _Callback, int ms, bool is_interval)
    {
      if (!is_running)
        return;
      __t.expires_after(std::chrono::milliseconds(ms));
      __t.async_wait([this, _Callback, ms, is_interval](std::error_code ec)
                     {
                    if (ec || !is_running) return;
                    _Callback();
                    if(is_interval)
                      start(_Callback, ms, is_interval); });
    };
  };

  timer timeinterval(asio::io_context &ctx, std::function<void(void)> _Callback, int ms)
  {
    return timer(ctx, _Callback, ms, true);
  }

  timer timeout(asio::io_context &ctx, std::function<void(void)> _Callback, int ms)
  {
    return timer(ctx, _Callback, ms, false);
  }
}
