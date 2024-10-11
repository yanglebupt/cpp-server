#pragma once

#include "asio.hpp"
#include "net_message.hpp"
#include "../utils/tsqueue.hpp"
#include <memory>
#include <iostream>

namespace net
{
  enum class error_code
  {
    write_validation_res_error,
    read_validation_res_error,
    bad_accepted_error,
    write_accepted_error,
    read_accepted_error,
    read_validation_error,
    write_validation_error,
    bad_validation_error,
    read_header_error,
    read_body_error,
    write_header_error,
    write_body_error,
    connection_error,
  };

  enum class owner_type
  {
    server,
    client
  };

  /**
   * 用来控制 p2p data transfer，全部都是异步操作
   * 创建时机：
   * - 当客户端发送请求到服务端，此时 owner 是 client
   * - 当服务端监听到客户端的请求，此时 owner 是 server
   */
  template <typename T, typename Connection>
  class connection : public std::enable_shared_from_this<connection<T, Connection>>
  {
  protected:
    // each connection has an unique socket to remote
    asio::ip::tcp::socket socket;
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
      // 由于这里只涉及从队列读数据，不存在竞争问题，因此队列返回引用即可
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
          OnError(error_code::write_header_error);
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
          OnError(error_code::write_body_error);
        } });
    };

    void ReadHeader()
    {
      // 这里涉及到写了，后面迁移多线程可能存在数据竞争，因此每次写都需要在线程的局部变量上写
      // 这里使用智能指针，自动释放内存，如果自己手动 new ，在后续很容易忘记 delete（或者出现多次 delete，智能指针可以多次 reset）
      // 例如出现错误的时候，需要手动 delete，但是智能指针的话，如果在回调函数中判断错误（不会继续向下传递）
      // 那么函数结束计数直接为 0，自动释放了
      std::shared_ptr<message<T>> msg = std::make_shared<message<T>>();
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
          OnError(error_code::read_header_error);
        } });
    };

    void ReadBody(std::shared_ptr<message<T>> msg)
    {
      asio::async_read(socket, asio::buffer(msg->body.data(), msg->body.size()), [this, msg](std::error_code ec, std::size_t length)
                       {
        if (!ec) {
          PushMessageToQueue(msg);
        } else {
          std::cout << "[" << id << "] Read Body Failed" << std::endl;
          OnError(error_code::read_body_error);
        } });
    };

    // 一个完整报文读取完毕
    void PushMessageToQueue(std::shared_ptr<message<T>> msg)
    {
      message_in_dq.emplace_back(PackMessage(msg)); // 将亡值，直接走 move
      msg.reset();                                  // 不调用 reset，函数执行完毕，也会自动释放
      // 继续读
      ReadHeader();
    };

    // 必须将 message 移动出来
    virtual owned_message<T, Connection> PackMessage(const std::shared_ptr<message<T>> msg) = 0;
    // 错误向上抛出
    virtual void OnError(error_code ecode) {};

  public:
    // 是否即将释放，如果为 true 则不能再继续注册回调了
    bool will_released = false;

    connection(asio::ip::tcp::socket socket, tsqueue<owned_message<T, Connection>> &qIn) : socket(std::move(socket)), message_in_dq(qIn) {};

    virtual ~connection()
    {
      std::cout << "[" << id << "] Connection Destroy, Closed" << std::endl;
      socket.close();
    };

    uint32_t GetID() const { return id; }

    bool Send(const message<T> &msg, bool move = true)
    {
      if (!IsConnected())
      {
        std::cout << "[" << id << "] Will Released! Can't send messages anymore" << std::endl;
        return false;
      }
      // 由于这里是异步，因此需要 copy，否则在异步回调执行中就拿不到引用了，如果外面不需要用了，这里可以 move
      // 当然你可以去写重载函数
      asio::post(this->socket.get_executor(), [this, inner_msg = move ? std::move(const_cast<message<T> &>(msg)) : msg]() mutable
                 {
                  bool isIdle = message_out_dq.empty();
                  // 往 out mesaage queue 添加要发送的消息，注意这里的 msg 是右值引用（左值），需要用 std::move 变成右值
                  message_out_dq.emplace_back(std::move(inner_msg));
                  // 如果在添加消息之前，队列为空，说明此时是空闲的，因此需要唤起任务
                  // 否则，发送消息的任务已经启动了，不需要再次启动
                  if (isIdle)
                    WriteHeader(); });
      return true;
    };

    bool IsConnected() const
    {
      return socket.is_open() && !will_released;
    };
  };

}
