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
      // 这里不能使用局部变量，局部变量在异步可能释放，但是这里会出现频繁取第一个元素，导致多次拷贝
      // 由于这里只涉及读，不存在竞争问题，因此队列返回引用即可
      // 写要保证顺序，能迁移到多线程写吗？比如一个线程写第一个，另一个线程写第二个，如果第一个写失败了咋办，第二个已经发出去了？
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
            message_out_dq.remove_front();
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
          message_out_dq.remove_front();
          if (!message_out_dq.empty())
            WriteHeader();
        } else {
          std::cout << "[" << id << "] Write Body Failed" << std::endl;
          socket.close();
        } });
    };

    void ReadHeader()
    {
      // 这里涉及到写了，后面迁移多线程可能存在数据竞争，因此每次写都需要在线程的局部变量上写
      message<T> *msg = new message<T>();
      asio::async_read(socket, asio::buffer(&msg->header, sizeof(message_header<T>)), [this, msg](std::error_code ec, std::size_t length)
                       {
        if (!ec) {
          if (msg->header.size > 0)
          {
            // 有 body
            msg->body.resize(msg->header.size);
            ReadBody(msg);
          }
          else
          {
            // 无 body
            PushMessageToQueue(msg);
          }
        } else {
          std::cout << "[" << id << "] Read Header Failed" << std::endl;
          // 这里可以调用离线，这样不用发消息时才确定离线，读取 Header 失败，一定是离线导致吗？
          socket.close();
        } });
    };

    void ReadBody(message<T> *msg)
    {
      asio::async_read(socket, asio::buffer(msg->body.data(), msg->body.size()), [this, msg](std::error_code ec, std::size_t length)
                       {
        if (!ec) {
          PushMessageToQueue(msg);
        } else {
          std::cout << "[" << id << "] Read Body Failed" << std::endl;
          socket.close();
        } });
    };

    virtual owned_message<T, Connection> PackMessage(message<T> *msg) = 0;

    // 一个完整报文读取完毕
    void PushMessageToQueue(message<T> *msg)
    {
      message_in_dq.emplace_back(PackMessage(msg)); // 将亡值，直接走 move
      delete msg;
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

    void Send(const message<T> &msg)
    {
      // 由于这里是异步，因此需要 copy，否则在异步回调执行中就拿不到引用了，如果外面不需要用了，这里可以 move
      // 当然你可以去写重载函数
      asio::post(ctx, [this, msg = std::move(const_cast<message<T> &>(msg))]() mutable
                 {
                   bool isIdle = message_out_dq.empty();
                   // 往 out mesaage queue 添加要发送的消息，注意这里的 msg 是右值引用（左值），需要用 std::move 变成右值
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