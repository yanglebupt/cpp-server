#pragma once

#include "asio.hpp"
#include "net_client_connection.hpp"
#include "../utils/logger.hpp"
#include <thread>
#include <memory>
#include <iostream>

namespace net
{
  template <typename T>
  class client_interface
  {
    friend class client_connection<T>;

  public:
    // 0 代表不进行重连
    int max_retries;
    // 0 代表不等待
    int retry_wait_ms;
    client_interface() : client_interface(0, 0) {};
    client_interface(int max_retries) : client_interface(max_retries, 0) {};
    client_interface(int max_retries, int retry_wait_ms) : max_retries(max_retries), retry_wait_ms(retry_wait_ms)
    {
      logger::cfg.show_time = true;
      logger::cfg.show_thread_id = true;
      logger::cfg.show_separator = true;
      logger::cfg.show_trace = false;
      logger::cfg.enable_save = true;
      logger::cfg.save_override = true;
      logger::cfg.external_log = true;
      t_log = std::thread(&logger::ex_log);
      logger::enable_setting();
    };

    virtual ~client_interface()
    {
      Stop();
      logger::terminate();
      // 因为读失败，不会继续添加读任务，写队列为空，也不会添加写任务，因此 io_context 会直接退出，不需要手动调用 stop
      // 注意不能在线程注册任务的回调函数里面，调用 join 函数, 也就是自己 join 自己，或者两个或多个线程互相 join
      if (ctx_thread.joinable())
        ctx_thread.join();
      if (t_log.joinable())
        t_log.join();
      logger::cfg.external_log = false;
    };

    // 只支持调用一次，不会维护发送到任意服务器的连接，连接不同的服务器，应该重新 new 一个 client，用另一个线程处理
    bool Connect(const std::string &host, const uint16_t &port)
    {
      try
      {
        // 用于处理 DNS 解析的一个组件。它可以将主机名（例如 "www.example.com"）转换为 IP 地址，或者将服务名（例如 "http"）转换为端口号。
        asio::ip::tcp::resolver resolver(ctx);
        // 上面获得的端点列表可能同时包含 IPv4 和 IPv6 端点，因此我们需要尝试其中的每一个，直到找到一个有效的端点。这使客户端程序独立于特定的 IP 版本。asio::connect() 函数会自动为我们执行此操作
        endpoints = resolver.resolve(host, std::to_string(port));

        // 创建连接
        m_connection = std::make_shared<client_connection<T>>(this, asio::ip::tcp::socket(ctx), message_in_dq);
        m_connection->ConnectToServer(endpoints, max_retries, retry_wait_ms);

        // 监听退出信号
        exit_signals.async_wait([this](auto, auto)
                                { Close(); });

        info("Start Connecting...");

        // 开始异步操作
        ctx_thread = std::thread([this]()
                                 { ctx.run(); });

        return true;
      }
      catch (const std::exception &e)
      {
        err("Client Exception: %s", e.what());
        return false;
      }
    }

    void Close()
    {
      if (m_connection != nullptr)
        m_connection->Close();
    }

    void Send(const message<T> &msg)
    {
      if (m_connection != nullptr)
        m_connection->Send(msg);
    }

    void Send(const byte_buffer &msg)
    {
      if (m_connection != nullptr)
        m_connection->Send(msg);
    }

    tsqueue<owned_message<T, client_connection<T>>> &InComing()
    {
      return message_in_dq;
    }

    /*--------------- 一些回调函数，不同的业务服务，可以有不同的回调函数 ----------------*/
  protected:
    void Stop()
    {
      std::unique_lock<std::mutex> lock(stop_mtx);
      if (will_stopped)
        return;
      will_stopped = true;
      ctx.stop();
      m_connection.reset();
      OnDisConnect();
    };

    virtual void ConnectionFailed(error_code ecode) {}
    virtual void Connected() {}
    virtual void OnError(error_code ecode) {}
    virtual void OnDisConnect() {}

    asio::io_context ctx;
    std::thread ctx_thread;
    std::shared_ptr<client_connection<T>> m_connection;
    // incoming message queue from server, and client need handle message in this queue
    tsqueue<owned_message<T, client_connection<T>>> message_in_dq;

  private:
    bool will_stopped = false;
    asio::ip::tcp::resolver::results_type endpoints;
    std::thread t_log;
    std::mutex stop_mtx;
    asio::signal_set exit_signals = asio::signal_set(ctx, SIGINT, SIGTERM, SIGBREAK);
  };

}
