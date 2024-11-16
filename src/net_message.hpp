#pragma once

#include "utils/serialization/serializable.hpp"
#include "utils/serialization/data_stream.hpp"

struct net_header : public serializable
{
  // body size in bytes
  len_t size = 0;
};

// T must extend from net_header
template <typename T>
struct net_message : public data_stream
{
  // struct 字节数：https://blog.csdn.net/liunanya/article/details/102863416/
  // 字节序对齐问题：https://blog.csdn.net/qq_26460507/article/details/77620523
  // 序列化 net_header: serializable 是不包含为了字节对齐而填充的字节
  T header;
  data_stream body;

  net_message() : data_stream(), body() {}
  net_message(Endian endian) : data_stream(endian), body(endian) {}

  void set_header_from_buffer(const std::vector<byte_t> &data)
  {
    data_stream ds(get_endian());
    ds.set_from_buffer(data);
    ds >> this->header;
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
  net_message<T> &operator<<(B obj)
  {
    body << obj;
    header.size = body.get_buffer().size();
    return *this;
  }

  template <typename B>
  net_message<T> &operator>>(B &obj)
  {
    body >> obj;
    return *this;
  }

  byte_buffer &get_buffer() override
  {
    data_stream::operator<<(header);
    // header size in bytes
    len_t header_size = header.__size;
    if (buffer.size() != header_size)
      throw std::runtime_error("header must be struct with primitive type");
    buffer.resize(header_size + header.size);
    memcpy(buffer.data() + header_size, body.get_buffer().data(), header.size);
    return buffer;
  }
};
