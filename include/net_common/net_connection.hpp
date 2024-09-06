#pragma once

#include "net_message.hpp"
#include "net_tsqueue.hpp"
#include "asio.hpp"
#include <iostream>

namespace net
{

  template <typename T>
  class server_interface;

  /**
   * 用来控制 p2p data transfer，全部都是异步操作
   * 创建时机：
   * - 当客户端发送请求到服务端，此时 owner 是 client
   * - 当服务端监听到客户端的请求，此时 owner 是 server
   */
  template <typename T>
  class connection : public std::enable_shared_from_this<connection<T>>
  {
  public:
    // server 和 client 的 connection，行为稍有不同
    enum class owner
    {
      server,
      client
    };

  protected:
    // each connection has an unique socket to remote
    asio::ip::tcp::socket socket;

    // 异步上下文是来自连接的 owner
    asio::io_context &ctx;

    // This queue holds all messages to be sent to the remote side of this connection
    // 确保顺序发送
    tsqueue<message<T>> message_out_dq;

    // This references the incoming queue of the owner of connection, we will push received message into this queue
    tsqueue<owned_message<T>> &message_in_dq;

    // The "owner" decides how some of the connection behaves
    owner ownerType;

    // 连接的唯一 ID
    uint32_t id;

    // 连接的验证码，可以一段时间更新一次，不要每个请求都去更新，太耗时了

    // validation 对于 client 来说是发送给服务端的响应码，对于 server 来说是发送给客户端的验证码
    uint64_t validation = 0;
    // validation 对于 client 来说是接收到服务端的验证码，对于 server 来说是接收到客户端的响应码
    uint64_t response = 0;
    // 客户端，该字段始终为 0
    uint64_t exceptResponseValidation = 0;

    message<T> tempMsg;

  private:
    // "Encrypt" Validation data
    uint64_t scramble(uint64_t nInput)
    {
      uint64_t out = nInput ^ 0xDEADBEEFC0DECAFE;
      out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0x0F0F0F0F0F0F0F) << 4;
      return out ^ 0xC0DEFACE12345678;
    }

    void UpdateValidation()
    {
      if (ownerType == owner::server)
      {
        // 服务器产生
        validation = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
        response = 0;
        exceptResponseValidation = scramble(validation);
      }
      else
      {
        validation = 0;
        response = 0;
        exceptResponseValidation = 0;
      }
    }

    void ReadValidation(server_interface<T> *server = nullptr)
    {
      asio::async_read(socket, asio::buffer(&response, sizeof(uint64_t)), [this, server](std::error_code ec, std::size_t length)
                       {
        if (!ec) {
          if (ownerType == owner::client) {
            validation = scramble(response);
            WriteValidation();
          } else {
            if (response == exceptResponseValidation) {
              std::cout << "[" << id << "] Validation OK" << std::endl;
              server->OnClientValidated(this->shared_from_this());
              ReadHeader();
            } else {
              std::cout << "[" << id << "] Validation Failed, Close" << std::endl;
              socket.close();
            }
          }
        } else {
          std::cout << "[" << id << "] Read Validation Failed" << std::endl;
          socket.close();
        } });
    }

    void WriteValidation()
    {
      asio::async_write(socket, asio::buffer(&validation, sizeof(uint64_t)), [this](std::error_code ec, std::size_t length)
                        {
        if (!ec) {
          if(ownerType == owner::client)
            ReadHeader();
        } else {
          std::cout << "[" << id << "] Write Validation Failed" << std::endl;
          socket.close();
        } });
    }

    void WriteHeader()
    {
      asio::async_write(socket, asio::buffer(&message_out_dq.front().header, sizeof(message_header<T>)), [this](std::error_code ec, std::size_t length)
                        {
        if (!ec) {
          if (message_out_dq.front().body.size() > 0)
          {
            // 有 body
            WriteBody();
          }
          else
          {
            // 无 body，写完毕
            message_out_dq.pop_front();
            if (!message_out_dq.empty()) WriteHeader();
          }
        } else {
          std::cout << "[" << id << "] Write Header Failed" << std::endl;
          socket.close();
        } });
    };

    void WriteBody()
    {
      asio::async_write(socket, asio::buffer(message_out_dq.front().body.data(), message_out_dq.front().body.size()), [this](std::error_code ec, std::size_t length)
                        {
        if (!ec) {
          message_out_dq.pop_front();
          if (!message_out_dq.empty())
            WriteHeader();
        } else {
          std::cout << "[" << id << "] Write Body Failed" << std::endl;
          socket.close();
        } });
    };

    void ReadHeader()
    {
      asio::async_read(socket, asio::buffer(&tempMsg.header, sizeof(message_header<T>)), [this](std::error_code ec, std::size_t length)
                       {
        if (!ec) {
          if (tempMsg.header.size > 0)
          {
            // 有 body
            tempMsg.body.resize(tempMsg.header.size);
            ReadBody();
          }
          else
          {
            // 无 body
            AddTempMsgToQueue();
          }
        } else {
          std::cout << "[" << id << "] Read Header Failed" << std::endl;
          // 这里可以调用离线，这样不用发消息时才确定离线，读取 Header 失败，一定是离线导致吗？
          socket.close();
        } });
    };

    void ReadBody()
    {
      asio::async_read(socket, asio::buffer(tempMsg.body.data(), tempMsg.body.size()), [this](std::error_code ec, std::size_t length)
                       {
        if (!ec) {
          AddTempMsgToQueue();
        } else {
          std::cout << "[" << id << "] Read Body Failed" << std::endl;
          socket.close();
        } });
    };

    void AddTempMsgToQueue()
    {
      // 一个完整报文读取完毕
      /**
       * 如果是 owner::client，说明 connection 是客户端向服务端发起请求时创建的
       * 并且是从 ConnectToServer 中的 ReadHeader 到这里，意味着数据来自服务端，这个 connection 不需要保存
       *
       *
       * 反之，说明 connection 是服务端接收到客户端发起请求时创建的，那么需要保存下来，方便服务端向该请求的客户端发送响应
       **/
      auto message_owner = ownerType == owner::client ? nullptr : this->shared_from_this();
      message_in_dq.emplace_back({message_owner, std::move(tempMsg)});

      // 继续读
      ReadHeader();
    }

  public:
    connection(owner ownerType, asio::io_context &ctx, asio::ip::tcp::socket socket, tsqueue<owned_message<T>> &qIn) : ownerType(ownerType), ctx(ctx), socket(std::move(socket)), message_in_dq(qIn)
    {
      UpdateValidation();
    };
    virtual ~connection() {};

    uint32_t GetID() const { return id; }

    void ConnectToClient(server_interface<T> *server, uint32_t serverClientID)
    {
      if (ownerType == owner::server)
      {
        if (socket.is_open())
        {
          id = serverClientID;
          // 向客户端发送验证码
          WriteValidation();

          // 等待客户端返回响应码，内部校验通过，开始读取 Header
          ReadValidation(server);
        }
      }
    }

    void ConnectToServer(const asio::ip::tcp::resolver::results_type &endpoints)
    {
      if (ownerType == owner::client)
      {
        asio::async_connect(socket, endpoints, [this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
                            {
          if(!ec){
            // 接收服务端的验证码，计算响应码，并返回给服务端，就可以读取 Header 了
            ReadValidation();
          } });
      }
    };

    void Send(const message<T> &msg)
    {
      asio::post(ctx, [this, msg]()
                 {
                  bool isIdle = message_out_dq.empty();
                  // 往 out mesaage queue 添加要发送的消息 
                  message_out_dq.emplace_back(std::move(msg));
                  // 如果在添加消息之前，队列为空，说明此时是空闲的，因此需要唤起任务
                  // 否则，发送消息的任务已经启动了，不需要再次启动
                  if (isIdle)
                    WriteHeader(); });
    };

    void DisConnect()
    {
      if (IsConnected())
        asio::post(ctx, [this]()
                   { socket.close(); });
    };

    bool IsConnected() const
    {
      return socket.is_open();
    };
  };

}
