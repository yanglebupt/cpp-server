#pragma once

#include "net_connection.hpp"
#include "net_message.hpp"
#include "net_tsqueue.hpp"
#include "functional"
#include <mutex>

namespace net
{
  template <typename T, typename Connection>
  // 逻辑类是用来处理消息队列，目前做法开辟一个线程处理全部消息
  class logic_system
  {
  protected:
    // incoming messages from remote
    tsqueue<owned_message<T, Connection>> message_in_dq;
    // handle incoming messages thread
    std::thread message_thread;

    void HandleMessages()
    {
      while (true)
      {
        message_in_dq.wait();
        owned_message<T, Connection> msg = message_in_dq.pop_front();
        OnMessage(msg.remote, msg.msg);
      }
    }

  public:
    virtual void OnMessage(std::shared_ptr<Connection> client, const message<T> &msg) = 0;
    bool StartHandleMessages()
    {
      message_thread = std::thread(&HandleMessages, this);
      return true;
    }

    virtual void Join()
    {
      if (message_thread.joinable())
        message_thread.join();
    }

    tsqueue<owned_message<T, Connection>> &InComing()
    {
      return message_in_dq;
    }

  public:
    virtual ~logic_system()
    {
      if (message_thread.joinable())
        message_thread.join();
      message_in_dq.free();
    };
  };
}