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
    bool _exit = false;

    void HandleMessages()
    {
      // 这里需要判断服务器是否退出
      while (!_exit)
      {
        message_in_dq.wait();

        if (_exit) // 即将退出，取出剩余的消息进行处理，再结束
        {
          while (!message_in_dq.empty())
          {
            owned_message<T, Connection> msg = message_in_dq.pop_front();
            this->OnMessage(msg.remote, msg.msg);
          }
          // 处理完毕，退出外层 while 循环，消息线程结束了
          break;
        }
        // 正常处理消息，取一个即可，将亡值 move 延长生命周期
        owned_message<T, Connection> msg = message_in_dq.pop_front();
        this->OnMessage(msg.remote, msg.msg);
        // 消息处理完，生命周期结束，消息自动释放
      }
      // 消息线程结束了
    }

  public:
    virtual void OnMessage(std::shared_ptr<Connection> client, const message<T> &msg) = 0;

    void StartHandleMessages()
    {
      message_thread = std::thread(&HandleMessages, this);
    }

    tsqueue<owned_message<T, Connection>> &InComing()
    {
      return message_in_dq;
    }

  public:
    virtual ~logic_system()
    {
      _exit = true;
      // 通知队列不要等待了
      message_in_dq.try_exit();
      // 等待剩余消息处理完毕
      if (message_thread.joinable())
        message_thread.join();
      std::cout << "[SERVER] Logic Systerm Exited!" << std::endl;
    };
  };
}