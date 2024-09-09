#pragma once

#include "asio.hpp"
#include "net_message.hpp"
#include "net_tsqueue.hpp"
#include <memory>
#include <iostream>

namespace net
{
  /**
   * 用来控制 p2p data transfer，全部都是异步操作
   * 创建时机：
   * - 当客户端发送请求到服务端，此时 owner 是 client
   * - 当服务端监听到客户端的请求，此时 owner 是 server
   */
  template <typename T, typename Connection>
  class connection : public std::enable_shared_from_this<connection<T, Connection>>
  {
  public:
    enum class owner_type
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
    tsqueue<owned_message<T, Connection>> &message_in_dq;

    // 连接的唯一 ID
    uint32_t id = 0;

    // 谁创建了这个连接
    owner_type owner;

    // "Encrypt" Validation data
    uint64_t scramble(uint64_t nInput)
    {
      uint64_t out = nInput ^ 0xDEADBEEFC0DECAFE;
      out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0x0F0F0F0F0F0F0F) << 4;
      return out ^ 0xC0DEFACE12345678;
    }

    void WriteHeader()
    {
      auto message = message_out_dq.front();
      asio::async_write(socket, asio::buffer(&message.header, sizeof(message_header<T>)), [this, &message](std::error_code ec, std::size_t length)
                        {
        if (!ec) {
          if (message.body.size() > 0)
          {
            // 有 body
            WriteBody(message);
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

    void WriteBody(message<T> &message)
    {
      asio::async_write(socket, asio::buffer(message.body.data(), message.body.size()), [this](std::error_code ec, std::size_t length)
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
      message<T> message;
      asio::async_read(socket, asio::buffer(&message.header, sizeof(message_header<T>)), [this, &message](std::error_code ec, std::size_t length)
                       {
        if (!ec) {
          if (message.header.size > 0)
          {
            // 有 body
            message.body.resize(message.header.size);
            ReadBody(message);
          }
          else
          {
            // 无 body
            PushMessageToQueue(message);
          }
        } else {
          std::cout << "[" << id << "] Read Header Failed" << std::endl;
          // 这里可以调用离线，这样不用发消息时才确定离线，读取 Header 失败，一定是离线导致吗？
          socket.close();
        } });
    };

    void ReadBody(message<T> &message)
    {
      asio::async_read(socket, asio::buffer(message.body.data(), message.body.size()), [this, &message](std::error_code ec, std::size_t length)
                       {
        if (!ec) {
          PushMessageToQueue(message);
        } else {
          std::cout << "[" << id << "] Read Body Failed" << std::endl;
          socket.close();
        } });
    };

    virtual owned_message<T, Connection> PackMessage(message<T> &message) = 0;

    // 一个完整报文读取完毕
    void PushMessageToQueue(message<T> &message)
    {
      message_in_dq.emplace_back(PackMessage(message));
      // 继续读
      ReadHeader();
    };

  public:
    connection(asio::io_context &ctx, asio::ip::tcp::socket socket, tsqueue<owned_message<T, Connection>> &qIn) : ctx(ctx), socket(std::move(socket)), message_in_dq(qIn) {};

    virtual ~connection()
    {
      std::cout << "connection destroy" << std::endl;
    };

    uint32_t GetID() const { return id; }

    void Send(message<T> &msg)
    {
      // 引用不是 const
      asio::post(ctx, [this, &msg]()
                 {
                  bool isIdle = message_out_dq.empty();
                  // 往 out mesaage queue 添加要发送的消息
                  message_out_dq.emplace_back(msg);
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
