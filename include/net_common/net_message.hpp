#pragma once

#include <stddef.h>
#include <ostream>
#include <vector>
#include <cstring>
#include <memory>

namespace net
{
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

    message() {}
    message(const message<T> &other)
    {
      header = other.header;
      body = other.body;
    }
    message(message<T> &&other)
    {
      header = other.header;
      body = std::move(other.body);
      other.header.size = 0;
    }

    // 返回整个消息包体的 bytes
    size_t size() const
    {
      return body.size();
    }

    // print
    friend std::ostream &operator<<(std::ostream &os, const message &message)
    {
      os << "ID: " << int(message.header.id) << ", Size: " << message.header.size;
      return os;
    }

    /**
     * write data to message body data tail, type <DataType> must simple so that it can be serialized
     * example:
     * >>> message<T> msg;
     * >>> msg << 1 << 0 << "sdsd";
     **/
    template <typename DataType>
    friend message<T> &operator<<(message<T> &msg, const DataType &data)
    {
      static_assert(std::is_standard_layout<DataType>::value, "Data is too complex and it cannot be serialized");

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

    /**
     * fetch data from message body data tail, type <DataType> must simple so that it can be serialized
     * example:
     * >>> message<T> msg;
     * >>> float x, y;
     * >>> msg >> x >> y;
     **/
    template <typename DataType>
    friend message<T> &operator>>(message<T> &msg, DataType &data)
    {
      static_assert(std::is_standard_layout<DataType>::value, "Data is too complex and it cannot be serialized");

      size_t ori_body_size = msg.body.size();

      // reduce body data size
      size_t new_body_size = ori_body_size - sizeof(DataType);

      std::memcpy(&data, msg.body.data() + new_body_size, sizeof(DataType));

      msg.body.resize(new_body_size);

      // recalculate the total message size
      msg.header.size = msg.size();

      return msg;
    }
  };

  // Forward declarations only for pointer, 因为指针类型分配的大小的确定的
  template <typename T>
  class connection;

  /**
   * 携带消息的主体：谁发来的消息
   */
  template <typename T>
  struct owned_message
  {
    /**
     * 消息发送者指针
     */
    std::shared_ptr<connection<T>> remote = nullptr;
    /**
     * 发送的消息
     */
    message<T> msg;

    // print
    friend std::ostream &operator<<(std::ostream &os, const owned_message<T> &msg)
    {
      os << msg.msg;
      return os;
    }
  };
}
