#pragma once

#include <stddef.h>
#include <ostream>
#include <vector>
#include <cstring>
#include <memory>
#include <iostream>
#include <type_traits>
#include <stdexcept>
#include "json/json.h"

namespace net
{
  class message_io_error : public std::exception
  {
  private:
    std::string __arg;

  public:
    message_io_error(const std::string &__arg) : __arg(__arg) {}
    const char *what() const throw() override
    {
      std::string message = "bad message io exception: ";
      message += __arg;
      return message.c_str();
    }
  };
  /**
   * 消息头
   */
  template <typename T>
  struct message_header
  {
    /**
     * 消息类型
     */
    T id;
    /**
     * 整个消息包体的 bytes，不包括消息头
     */
    uint32_t size = 0;
  };

  template <typename T>
  /**
   * 服务端和客户端传递的消息包
   */
  struct message
  {
    /**
     * 消息头
     */
    message_header<T> header;
    /**
     * 消息体
     */
    std::vector<uint8_t> body;

    inline static Json::FastWriter __json_writer = Json::FastWriter();
    inline static Json::Reader __json_reader = Json::Reader();

    void clear()
    {
      body.clear();
      std::vector<uint8_t>().swap(body);
      header.size = 0;
    }

    message() {}
    message(const message<T> &other)
    {
      header = other.header;
      body = other.body;
    }
    message<T> &operator=(const message<T> &other)
    {
      if (&other != this)
      {
        header = other.header;
        body = other.body;
      }
      return *this;
    }
    message(message<T> &&other)
    {
      header = other.header;
      other.header.size = 0;
      body = std::move(other.body);
    }
    message<T> &operator=(message<T> &&other)
    {
      if (&other != this)
      {
        header = other.header;
        other.header.size = 0;
        body = std::move(other.body);
      }
      return *this;
    }

    // 返回消息包体的 bytes
    size_t size() const
    {
      return body.size();
    }

    void print() const
    {
      for (const uint8_t &n : body)
      {
        std::cout << (int)n << ",";
      }
      std::cout << std::endl;
    }

    // print
    friend std::ostream &operator<<(std::ostream &os, const message &message)
    {
      os << "ID: " << int(message.header.id) << ", Size: " << message.header.size;
      return os;
    }

    /**
     * write data to message body data tail, type <DataType> must simple so that it can be serialized
     * sizeof(<DataType>) must be the contiguous memory size occupied by the data
     * example:
     * >>> message<T> msg;
     * >>> msg << 1 << 0 << "sdsd";
     **/
    template <typename DataType>
    friend message<T> &operator<<(message<T> &msg, const DataType &data)
    {
      if constexpr (std::is_same<DataType, std::string>::value ||
                    std::is_same<DataType, std::vector<uint8_t>>::value ||
                    std::is_same<DataType, std::vector<char>>::value)
      {
        msg.body.insert(msg.body.end(), data.begin(), data.end());
        msg.header.size = msg.size();
        return msg;
      }
      else if constexpr (std::is_same<DataType, message<T>>::value)
      {
        msg.body.insert(msg.body.end(), data.body.begin(), data.body.end());
        msg.header.size = msg.size();
        return msg;
      }
      else if constexpr (std::is_same<DataType, Json::Value>::value)
      {
        std::string str = message<T>::__json_writer.write(data);
        msg.body.insert(msg.body.end(), str.begin(), str.end());
        msg.header.size = msg.size();
        return msg;
      }
      else
      {
        // char[n] ，char[]，char* 不是一个类型
        // DataType 如果是 char[n]，那么 sizeof(DataType) 可以获取正常大小。但是不能是 char*
        static_assert(std::is_standard_layout<DataType>::value, "Data is too complex and it cannot be serialized");
        // 不允许指针类型
        static_assert(!std::is_pointer<DataType>::value, "can't append data in message by a pointer");

        size_t ori_body_size = msg.body.size();

        // increment body data size
        size_t new_body_size = ori_body_size + sizeof(DataType);
        msg.body.resize(new_body_size);

        // .data() return start pointer
        // + ori_body_size (offset) is the end pointer, &data return the data start pointer
        std::memcpy(msg.body.data() + ori_body_size, &data, sizeof(DataType));

        // recalculate the total message size
        msg.header.size = msg.size();

        return msg;
      }
    }

    /**
     * fetch data from message body data tail, type <DataType> must simple so that it can be serialized
     * sizeof(<DataType>) must be the contiguous memory size occupied by the data
     * example:
     * >>> message<T> msg;
     * >>> float x, y;
     * >>> msg >> x >> y;
     **/
    template <typename DataType>
    friend message<T> &operator>>(message<T> &msg, DataType &data)
    {
      // 将剩余数据全部取出来
      if constexpr (std::is_same<DataType, std::string>::value ||
                    std::is_same<DataType, std::vector<uint8_t>>::value ||
                    std::is_same<DataType, std::vector<char>>::value)
      {
        data.resize(msg.body.size());
        std::memcpy(data.data(), msg.body.data(), msg.body.size());
        msg.clear();
        return msg;
      }
      else if constexpr (std::is_same<DataType, message<T>>::value)
      {
        msg.body.swap(data.body);
        msg.clear();
        return msg;
      }
      else if constexpr (std::is_same<DataType, Json::Value>::value)
      {
        std::string str;
        str.resize(msg.body.size());
        std::memcpy(str.data(), msg.body.data(), msg.body.size());
        message<T>::__json_reader.parse(str, data);
        msg.clear();
        return msg;
      }
      else
      {
        static_assert(std::is_standard_layout<DataType>::value, "Data is too complex and it cannot be serialized");
        static_assert(!std::is_pointer<DataType>::value, "can't append data in message by a pointer");

        size_t ori_body_size = msg.body.size();

        // reduce body data size
        size_t new_body_size = ori_body_size - sizeof(DataType);

        std::memcpy(&data, msg.body.data() + new_body_size, sizeof(DataType));

        msg.body.resize(new_body_size);

        // recalculate the total message size
        msg.header.size = msg.size();

        return msg;
      }
    }
  };

  /**
   * 携带消息的主体：谁发来的消息
   */
  template <typename T, typename Connection>
  struct owned_message
  {
    /**
     * 消息发送者指针
     */
    std::shared_ptr<Connection> remote = nullptr;
    /**
     * 发送的消息
     */
    message<T> msg;

    owned_message() {};
    owned_message(const owned_message<T, Connection> &other)
    {
      remote = other.remote;
      msg = other.msg;
    }
    owned_message<T, Connection> &operator=(const owned_message<T, Connection> &other)
    {
      if (&other != this)
      {
        remote = other.remote;
        msg = other.msg;
      }
      return *this;
    }
    owned_message(owned_message<T, Connection> &&other)
    {
      remote = std::move(other.remote);
      msg = std::move(other.msg);
    }
    owned_message<T, Connection> &operator=(owned_message<T, Connection> &&other)
    {
      if (&other != this)
      {
        remote = std::move(other.remote);
        msg = std::move(other.msg);
      }
      return *this;
    }

    // print
    friend std::ostream &operator<<(std::ostream &os, const owned_message<T, Connection> &msg)
    {
      os << msg.msg;
      return os;
    }
  };
}
