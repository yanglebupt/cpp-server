#pragma once

#include "../utils/logic_systerm.hpp"
#include "../net_tools/io_context_pool.hpp"
#include "net_server_connection.hpp"
#include <iostream>
#include <map>

namespace net
{
  template <typename T>
  class server_interface : public logic_system<message<T>, server_connection<T>>
  {
  public:
    // 注意不能在构造方法里面 Start，要不然回调函数就不会走 override 了，必须等构造完成后手动 Start
    server_interface(std::uint16_t port) : m_acceptor(ctx, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {};

    virtual ~server_interface()
    {
      std::cout << "[SERVER] Stop!" << std::endl;
    }

    void Start()
    {
      try
      {
        io_context_pool::Instance()->Start();
        // 监听退出信号
        asio::signal_set exit_signals(ctx, SIGINT, SIGTERM);
        exit_signals.async_wait([this](auto, auto)
                                { ctx.stop(); io_context_pool::Instance()->Stop(); });
        // 开启消息子线程
        this->StartHandleMessages();
        // 一直循环监听请求
        WaitForClientConnection();

        std::cout << "[SERVER] Started! " << io_context_pool::Instance()->size << " threads handle" << std::endl;

        // 主线程堵塞
        ctx.run();
      }
      catch (const std::exception &e)
      {
        std::cerr << "[SERVER] Exception: " << e.what() << std::endl;
        throw e;
      }
    }

    void WaitForClientConnection()
    {
      // 监听客户端连接
      m_acceptor.async_accept(io_context_pool::Instance()->GetIOContext(), [this](std::error_code ec, asio::ip::tcp::socket socket)
                              {
        bool isAccepted = !ec;
        if (isAccepted)
        {
          std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << std::endl;
          // 这个 client 需要保留下来，后面服务器响应的时候要用到
          std::shared_ptr<server_connection<T>>
              client = std::make_shared<server_connection<T>>(this, std::move(socket), this->InComing());

          // 由具体的业务服务，确定该请求是否接收
          isAccepted = this->ShouldAcceptClient(client);
          client->ConnectToClient(nIDCounter++, isAccepted);

          m_connections.insert({client->GetID(), std::move(client)});
        }

        // 循环监听
        WaitForClientConnection(); });
    }

    // 返回当前的有效客户端连接数量
    uint16_t ClientCount() const
    {
      return m_connections.size();
    }

    void DisConnectClient(uint32_t clientID)
    {
      if (m_connections.count(clientID) < 1)
        return;
      std::shared_ptr<server_connection<T>> removed_client = m_connections.at(clientID);
      removed_client->will_released = true;
      m_connections.erase(clientID);
      this->OnClientDisConnect(removed_client);
    }

    void SendMessageAllClients(const message<T> &msg, std::shared_ptr<server_connection<T>> ignoreClient)
    {
      for (const std::pair<uint32_t, std::shared_ptr<server_connection<T>>> &pair : m_connections)
      {
        std::shared_ptr<server_connection<T>> client = pair.second;
        if (client == ignoreClient && client->GetID() == ignoreClient->GetID())
          continue;
        client->Send(msg, false);
      }
    }

    /*--------------- 一些回调函数，不同的业务服务，可以有不同的回调函数 ----------------*/
  public:
    // 客户端通过验证
    virtual void OnClientValidated(std::shared_ptr<server_connection<T>> client)
    {
    }

  protected:
    // 决定是否建立连接
    virtual bool ShouldAcceptClient(std::shared_ptr<server_connection<T>> client)
    {
      return true;
    }

    // 客户端断开连接，在调用这个函数之前，服务器已经移除了这个客户端，客户端个数已经减一了
    // 但是资源的释放需要等待该函数执行完毕
    virtual void OnClientDisConnect(std::shared_ptr<server_connection<T>> client)
    {
    }

    // 接收到客户端消息包
    virtual void OnMessage(std::shared_ptr<server_connection<T>> client, message<T> &msg) = 0;

    asio::io_context ctx;
    asio::ip::tcp::acceptor m_acceptor;
    // Container of active validated connections
    std::map<uint32_t, std::shared_ptr<server_connection<T>>> m_connections;

    // Clients will be identified in the "wider system" via an ID
    uint32_t nIDCounter = 10;
  };
}
