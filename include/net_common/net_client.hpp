#pragma once

#include "asio.hpp"
#include "net_message.hpp"
#include "net_tsqueue.hpp"
#include "net_client_connection.hpp"
#include <thread>
#include <memory>
#include <iostream>

namespace net
{
  template <typename T>
  class client_interface
  {
  public:
    client_interface() {};
    virtual ~client_interface()
    {
      DisConnect();
    };

    // 只支持调用一次，不会维护发送到任意服务器的连接，连接不同的服务器，应该重新 new 一个 client，用另一个线程处理
    bool Connect(const std::string &host, const uint16_t &port)
    {
      try
      {
        // 用于处理 DNS 解析的一个组件。它可以将主机名（例如 "www.example.com"）转换为 IP 地址，或者将服务名（例如 "http"）转换为端口号。
        asio::ip::tcp::resolver resolver(ctx);
        // 上面获得的端点列表可能同时包含 IPv4 和 IPv6 端点，因此我们需要尝试其中的每一个，直到找到一个有效的端点。这使客户端程序独立于特定的 IP 版本。asio::connect() 函数会自动为我们执行此操作
        asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

        // 创建连接
        m_connection = std::make_unique<client_connection<T>>(ctx, asio::ip::tcp::socket(ctx), message_in_dq);
        m_connection->ConnectToServer(endpoints);

        // 开始异步操作
        ctx_thread = std::thread([this]()
                                 { ctx.run(); });

        return true;
      }
      catch (const std::exception &e)
      {
        std::cerr << "Client Exception:" << e.what() << std::endl;
        return false;
      }
    }

    void DisConnect()
    {
      if (IsConnected())
        m_connection->DisConnect();

      ctx.stop();
      if (ctx_thread.joinable())
        ctx_thread.join();

      m_connection.reset();
    };

    bool IsConnected()
    {
      if (m_connection != nullptr)
        return m_connection->IsConnected();
      else
        return false;
    }

    // 只进行一次拷贝，不影响外面的消息，当然外面可以 std::move 传参进来
    void Send(message<T> msg)
    {
      if (IsConnected())
        m_connection->Send(msg);
    }

    tsqueue<owned_message<T, client_connection<T>>> &InComing()
    {
      return message_in_dq;
    }

  protected:
    asio::io_context ctx;
    std::thread ctx_thread;
    std::unique_ptr<client_connection<T>> m_connection;

  private:
    // incoming message queue from server, and client need handle message in this queue
    tsqueue<owned_message<T, client_connection<T>>> message_in_dq;
  };

}
