#pragma once

#include "net_logic_systerm.hpp"
#include "net_server_connection.hpp"
#include <iostream>

namespace net
{
  template <typename T>
  class server_interface : logic_system<T, server_connection<T>>
  {
  public:
    server_interface(std::uint16_t port) : m_acceptor(ctx, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {};

    virtual ~server_interface()
    {
      Stop();
    }

    uint16_t ClientCount() const
    {
      return m_connections_dq.size();
    }

    bool Start()
    {
      bool no_error = true;
      try
      {
        // 一直循环监听
        WaitForClientConnection();

        ctx_thread = std::thread([this]()
                                 { ctx.run(); });

        std::cout << "[SERVER] Started!" << std::endl;
        no_error = true;
      }
      catch (const std::exception &e)
      {
        std::cerr << "[SERVER] Exception: " << e.what() << std::endl;
        no_error = false;
      }

      if (no_error)
        this->StartHandleMessages();

      return no_error;
    }

    virtual void Join() override
    {
      if (ctx_thread.joinable())
        ctx_thread.join();
      logic_system<T, server_connection<T>>::Join();
    }

    void WaitForClientConnection()
    {
      // 监听客户端连接
      m_acceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket)
                              {
        bool isAccepted = !ec;
        if (isAccepted)
        {
          std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << std::endl;
          // 这个 client 需要保留下来，后面服务器响应的时候要用到
          std::shared_ptr<server_connection<T>>
              client = std::make_shared<server_connection<T>>(this, ctx, std::move(socket), this->InComing());

          // 由具体的业务服务，确定该请求是否接收
          isAccepted = OnClientConnect(client);
          if (isAccepted)
          {
            m_connections_dq.emplace_back(client);
            std::cout << "[-----] Connection Approved" << std::endl;

            // 背后添加一个异步任务，io_context 是在一个子线程中允许全部的异步任务吗，执行顺序是和添加顺序一致吗？
            // 在一个异步任务的结束时添加异步任务，添加的异步任务会立刻执行，还是在重新调度？
            client->ConnectToClient(nIDCounter++);
          }
        }

        if(!isAccepted)
        {
          std::cout << "[-----] Connection Denied" << std::endl;
        }

        // 循环监听
        WaitForClientConnection(); });
    }

    void Stop()
    {
      ctx.stop();
      if (ctx_thread.joinable())
        ctx_thread.join();
      std::cout << "[SERVER] Stop!" << std::endl;
    }

    /*--------------- 发送消息，注意不要自己手动调 client->Send(msg) ，必须先判断是否掉线了 ----------------*/
    void SendMessageClient(std::shared_ptr<server_connection<T>> &client, const message<T> &msg)
    {
      if (client->IsConnected())
      {
        client->Send(msg);
      }
      else
      {
        // TODO: client 下线了，目前的设计要服务器要向客户端发送消息，才能知道客户端下线了，并释放连接资源
        // 客户端下线，服务端无法立刻知道
        OnClientDisConnect(client);
        client.reset(); // 释放连接资源
        // 进行指针比较的时候，都是通过内部管理的指针进行比较
        // 先将元素移动到尾部，然后移除，效率更高
        m_connections_dq.erase(std::remove(m_connections_dq.begin(), m_connections_dq.end(), nullptr), m_connections_dq.end());
      }
    }

    void SendMessageAllClients(const message<T> &msg, std::shared_ptr<server_connection<T>> &ignoreClient)
    {
      bool removeInvalid = false;
      for (auto &client : m_connections_dq)
      {
        if (client->IsConnected())
        {
          if (client != ignoreClient)
            client->Send(msg);
        }
        else
        {
          // client 下线了
          OnClientDisConnect(client);
          client.reset();
          removeInvalid = true;
        }
      }

      if (removeInvalid)
        m_connections_dq.erase(std::remove(m_connections_dq.begin(), m_connections_dq.end(), nullptr), m_connections_dq.end());
    }

    /*--------------- 一些回调函数，不同的业务服务，可以有不同的回调函数 ----------------*/
  public:
    // 客户端通过验证
    virtual void OnClientValidated(std::shared_ptr<server_connection<T>> client)
    {
    }

  protected:
    // 决定是否建立连接
    virtual bool OnClientConnect(std::shared_ptr<server_connection<T>> client)
    {
      return true;
    }

    // 是否连接
    virtual void OnClientDisConnect(std::shared_ptr<server_connection<T>> client)
    {
    }

    // 接收到客户端消息包
    virtual void OnMessage(std::shared_ptr<server_connection<T>> client, const message<T> &msg) = 0;

    asio::io_context ctx;
    std::thread ctx_thread;
    asio::ip::tcp::acceptor m_acceptor;
    // Container of active validated connections
    std::deque<std::shared_ptr<server_connection<T>>> m_connections_dq;

    // Clients will be identified in the "wider system" via an ID
    uint32_t nIDCounter = 10000;
  };
}
