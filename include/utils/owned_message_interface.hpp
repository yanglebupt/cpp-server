#pragma once

#include <memory>

template <typename T, typename Connection>
struct owned_message_interface
{
  /**
   * 消息发送者指针
   */
  std::shared_ptr<Connection> remote = nullptr;
  /**
   * 发送的消息
   */
  T msg;

  virtual ~owned_message_interface() {}
  owned_message_interface() {};
  owned_message_interface(const owned_message_interface<T, Connection> &other)
  {
    remote = other.remote;
    msg = other.msg;
  }
  owned_message_interface<T, Connection> &operator=(const owned_message_interface<T, Connection> &other)
  {
    if (&other != this)
    {
      remote = other.remote;
      msg = other.msg;
    }
    return *this;
  }
  owned_message_interface(owned_message_interface<T, Connection> &&other)
  {
    remote = std::move(other.remote);
    msg = std::move(other.msg);
  }
  owned_message_interface<T, Connection> &operator=(owned_message_interface<T, Connection> &&other)
  {
    if (&other != this)
    {
      remote = std::move(other.remote);
      msg = std::move(other.msg);
    }
    return *this;
  }
};