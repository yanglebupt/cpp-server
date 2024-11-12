#pragma once

#include "../utils/logic_systerm.hpp"
#include "../net_tools/io_context_pool.hpp"
#include "net_server_connection.hpp"
#include <map>
#include <sstream>

namespace net
{
  template <typename T>
  class server_interface : public logic_system<message<T>, server_connection<T>>
  {
    friend class server_connection<T>;

  private:
    std::thread t_log;
    bool will_closed = false;

    void AddClient(std::shared_ptr<server_connection<T>> client)
    {
      m_connections.insert({client->GetID(), std::move(client)});
    }

    void RemoveClient(uint32_t clientID)
    {
      if (will_closed || m_connections.count(clientID) < 1)
        return;
      std::shared_ptr<server_connection<T>> removed_client = m_connections.at(clientID);
      m_connections.erase(clientID);
      OnClientDisConnect(removed_client);
    }

    void RemoveClient(std::shared_ptr<server_connection<T>> client)
    {
      RemoveClient(client.GetID());
    }

    void WaitForClientConnection()
    {
      // 监听客户端连接
      m_acceptor.async_accept(io_context_pool::Instance()->GetIOContext(), [this](std::error_code ec, asio::ip::tcp::socket socket)
                              {
        bool isAccepted = !ec;
        if (isAccepted)
        {
          asio::ip::tcp::endpoint ed = socket.remote_endpoint();
          std::ostringstream address;
          address << ed;
          info("[SERVER] New Connection: %s", address.str().c_str());
          // 这个 client 需要保留下来，后面服务器响应的时候要用到
          std::shared_ptr<server_connection<T>>
              client = std::make_shared<server_connection<T>>(this, std::move(socket), this->InComing());

          // 由具体的业务服务，确定该请求是否接收
          isAccepted = this->ShouldAcceptClient(client);
          client->ConnectToClient(nIDCounter++, isAccepted);
        }

        // 循环监听
        WaitForClientConnection(); });
    }

  public:
    // 注意不能在构造方法里面 Start，要不然回调函数就不会走 override 了，必须等构造完成后手动 Start
    server_interface(std::uint16_t port) : m_acceptor(ctx, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
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

    virtual ~server_interface()
    {
      Close();
      io_context_pool::Instance()->Join();
      logger::terminate();
      if (t_log.joinable())
        t_log.join();
      logger::cfg.external_log = false;
      warn("[SERVER] Stop!");
    }

    // 注意 close 之后必须进行析构
    void Close()
    {
      if (will_closed)
        return;
      will_closed = true;
      // 关闭全部连接
      for (auto item : m_connections)
      {
        item.second->Close();
        OnClientDisConnect(item.second);
      }
      m_connections.clear();
      ctx.stop();
      io_context_pool::Instance()->Stop();
    }

    void Start()
    {
      try
      {
        io_context_pool::Instance()->Start();
        // 监听退出信号，永远存在的一个 work，直到 exit
        asio::signal_set exit_signals(ctx, SIGINT, SIGTERM);
        exit_signals.async_wait([this](auto, auto)
                                { Close(); });
        // 开启消息子线程
        this->StartHandleMessages();
        // 先注册了 async_accept 一直循环监听请求，那么一直存在任务
        // 判断是否有任务的逻辑是：回调执行完毕，才会移除该任务，然后再判断是否还有任务，没有则 io_context stop
        // 由于我们在回调函数里面，又注册了任务，因此 io_context 不会 stop 下来，除非你手动调用 stop
        WaitForClientConnection();

        info("[SERVER] Started! %d threads handle", io_context_pool::Instance()->size);

        // 主线程堵塞
        ctx.run();
      }
      catch (const std::exception &e)
      {
        err("[SERVER] Exception: %s", e.what());
        throw e;
      }
    }

    // 返回当前的有效客户端连接数量
    uint16_t ClientCount() const
    {
      return m_connections.size();
    }

    void SendMessageAllClients(const message<T> &msg, std::shared_ptr<server_connection<T>> ignoreClient)
    {
      for (const std::pair<uint32_t, std::shared_ptr<server_connection<T>>> &pair : m_connections)
      {
        std::shared_ptr<server_connection<T>> client = pair.second;
        if (ignoreClient == nullptr || (client == ignoreClient && client->GetID() == ignoreClient->GetID()))
          continue;
        client->Send(msg, false);
      }
    }

    /*--------------- 一些回调函数，不同的业务服务，可以有不同的回调函数 ----------------*/
  protected:
    // 决定是否建立连接
    virtual bool ShouldAcceptClient(std::shared_ptr<server_connection<T>> client)
    {
      return true;
    }

    // 客户端通过验证
    virtual void OnClientValidated(std::shared_ptr<server_connection<T>> client)
    {
    }

    // 客户端断开连接，在调用这个函数之前，服务器已经移除了这个客户端，客户端个数已经减一了
    // 但是资源的释放需要等待该函数执行完毕
    virtual void OnClientDisConnect(std::shared_ptr<server_connection<T>> client)
    {
    }

    // 接收到客户端消息包
    virtual void OnMessage(std::shared_ptr<server_connection<T>> client, message<T> &msg) = 0;

    // 未处理的异常
    virtual void OnError(error_code ecode) {}

    asio::io_context ctx;
    asio::ip::tcp::acceptor m_acceptor;
    // Container of active validated connections
    std::map<uint32_t, std::shared_ptr<server_connection<T>>> m_connections;

    // Clients will be identified in the "wider system" via an ID
    uint32_t nIDCounter = 10;
  };
}
