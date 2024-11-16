#pragma once

#include "../utils/serialization/serializable.hpp"
#include "../utils/serialization/data_stream.hpp"
#include "../utils/owned_message_interface.hpp"

namespace net
{
  struct header : public serializable
  {
    // body size in bytes
    len_t size = 0;
  };

  // T must extend from header
  template <typename T>
  struct message : public data_stream
  {
  private:
    void __init__(const message<T> &other)
    {
      header = other.header;
      body = other.body;
    };

    void __init__(message<T> &&other)
    {
      header = other.header;
      other.header.size = 0;
      body = std::move(other.body);
    };

  protected:
    data_stream body;
    bool has_write_buffer = false;

  public:
    // struct 字节数：https://blog.csdn.net/liunanya/article/details/102863416/
    // 字节序对齐问题：https://blog.csdn.net/qq_26460507/article/details/77620523
    // 序列化 header: serializable 是不包含为了字节对齐而填充的字节
    T header;

    message() : data_stream(), body() {}
    message(Endian endian) : data_stream(endian), body(endian) {}

    message(const message<T> &other)
    {
      __init__(other);
    }
    message<T> &operator=(const message<T> &other)
    {
      if (&other != this)
        __init__(other);
      return *this;
    }
    message(message<T> &&other)
    {
      __init__(std::forward<message<T>>(other));
    }
    message<T> &operator=(message<T> &&other)
    {
      if (&other != this)
        __init__(std::forward<message<T>>(other));
      return *this;
    }

    void set_header_from_buffer(const std::vector<byte_t> &data)
    {
      data_stream ds(get_endian());
      ds.set_from_buffer(data);
      ds >> this->header;
      body.get_buffer().resize(header.size);
    }

    void set_body_from_buffer(const std::vector<byte_t> &data)
    {
      body.set_from_buffer(data);
    }

    // B must extend from serializable
    template <typename B>
    void operator=(B obj)
    {
      body.clear();
      body << obj;
      header.size = body.get_buffer().size();
    }

    // B must extend from serializable
    template <typename B>
    message<T> &operator<<(B obj)
    {
      body << obj;
      header.size = body.get_buffer().size();
      return *this;
    }

    template <typename B>
    message<T> &operator>>(B &obj)
    {
      body >> obj;
      return *this;
    }

    // 确保所有的更新在 get_buffer 之前，可以做监听 body 的变化，但是没法监听 header 的变化
    byte_buffer &get_buffer() override
    {
      if (has_write_buffer)
        return buffer;
      data_stream::operator<<(header);
      len_t header_size = header.__size;
      if (buffer.size() != header_size)
        throw std::runtime_error("header must be struct with primitive type");
      buffer.resize(header_size + header.size);
      memcpy(buffer.data() + header_size, body.get_buffer().data(), header.size);
      has_write_buffer = true;
      return buffer;
    }

    // header size in bytes
    len_t get_header_size()
    {
      return header.__size;
    }

    byte_buffer &get_body_buffer()
    {
      return body.get_buffer();
    }
  };

  /**
   * 携带消息的主体：谁发来的消息
   */
  template <typename T, typename Connection>
  using owned_message = owned_message_interface<message<T>, Connection>;
}
