#pragma once
#include "byte_buffer.hpp"
#include "serializable.hpp"
#include <set>
#include <list>
#include <map>

class data_stream
{
private:
  void __init__(const data_stream &other)
  {
    rpos = other.rpos;
    buffer = other.buffer;
  };

  void __init__(data_stream &&other)
  {
    rpos = other.rpos;
    other.rpos = 0;
    buffer = std::move(other.buffer);
  };

public:
  data_stream() : buffer(device_endian) {}
  data_stream(Endian endian) : buffer(endian) {}
  ~data_stream() {}

  data_stream(const data_stream &other)
  {
    __init__(other);
  }
  data_stream &operator=(const data_stream &other)
  {
    if (&other != this)
      __init__(other);
    return *this;
  }
  data_stream(data_stream &&other)
  {
    __init__(std::forward<data_stream>(other));
  }
  data_stream &operator=(data_stream &&other)
  {
    if (&other != this)
      __init__(std::forward<data_stream>(other));
    return *this;
  }

  // 基本数据类型
  template <typename T>
  void write(T data)
  {
    buffer.write(data);
  }
  template <typename T>
  void read(T &data)
  {
    rpos += buffer.read(data, rpos);
  }

  // 字符串
  void write(const char *data)
  {
    len_t len = strlen(data);
    write(len);
    char data_cpy[len];
    std::memcpy(&data_cpy, data, len);
    buffer.write((byte_t *)&data_cpy, -1, len, false);
  }
  void write(const std::string &data)
  {
    write(data.c_str());
  }
  void read(std::string &data)
  {
    len_t len;
    read(len);
    data.assign((char *)&buffer[rpos], len);
    rpos += len;
  }

  // 写入容器
  template <typename T>
  void write(const std::vector<T> &data)
  {
    len_t len = data.size();
    write(len);
    for (auto it = data.begin(); it != data.end(); it++)
      write(*it);
  }
  template <typename T>
  void read(std::vector<T> &data)
  {
    len_t len;
    read(len);
    for (size_t i = 0; i < len; i++)
    {
      T value;
      read(value);
      data.push_back(std::move(value));
    }
  }

  // 写入KV容器
  template <typename K, typename V>
  void write(const std::map<K, V> &data)
  {
    len_t len = data.size();
    write(len);
    for (auto it = data.begin(); it != data.end(); it++)
    {
      write(it->first);
      write(it->second);
    }
  }
  template <typename K, typename V>
  void read(std::map<K, V> &data)
  {
    len_t len;
    read(len);
    for (size_t i = 0; i < len; i++)
    {
      K key;
      read(key);
      V value;
      read(value);
      data[key] = std::move(value);
    }
  }

  // 写入自定义可序列化类
  void write(const serializable &obj)
  {
    obj.serialize(*this);
  }
  void read(serializable &obj)
  {
    obj.deserialize(*this);
  }

  // 写入不定参数
  template <typename T, typename... Args>
  void write_args(const T &value, const Args &...args)
  {
    write(value);
    write_args(args...);
  }
  template <typename T>
  void write_args(const T &value)
  {
    write(value);
  };
  template <typename T, typename... Args>
  void read_args(T &value, Args &...args)
  {
    read(value);
    read_args(args...);
  }
  template <typename T>
  void read_args(T &value)
  {
    read(value);
  };

  // write stream
  template <typename T>
  data_stream &operator<<(T data)
  {
    if constexpr (std::is_base_of_v<byte_buffer, T>)
    {
      len_t o_size = buffer.size();
      len_t n_size = data.size();
      buffer.resize(o_size + n_size);
      memcpy(buffer.data() + o_size, data.data(), n_size);
    }
    else if constexpr (std::is_base_of_v<serializable, T>)
      write(dynamic_cast<serializable &>(data));
    else
      write(data);
    return *this;
  }
  template <typename T>
  data_stream &operator<<(const std::vector<T> &data)
  {
    write(data);
    return *this;
  }
  template <typename K, typename V>
  data_stream &operator<<(const std::map<K, V> &data)
  {
    write(data);
    return *this;
  }

  // read stream
  template <typename T>
  data_stream &operator>>(T &data)
  {
    if constexpr (std::is_base_of_v<serializable, T>)
      read(dynamic_cast<serializable &>(data));
    else
      read(data);
    return *this;
    return *this;
  }
  template <typename T>
  data_stream &operator>>(std::vector<T> &data)
  {
    read(data);
    return *this;
  }
  template <typename K, typename V>
  data_stream &operator>>(std::map<K, V> &data)
  {
    read(data);
    return *this;
  }

  // 保存
  void save(const char *filename)
  {
    buffer.save(filename);
  };
  // 加载
  void load(const char *filename)
  {
    buffer.load(filename);
  };

  // print
  friend std::ostream &operator<<(std::ostream &out, const data_stream &stream)
  {
    return out << stream.buffer;
  }

  // 继承的方法
  void clear()
  {
    buffer.clear();
    std::vector<byte_t>().swap(buffer);
    rpos = 0;
  }

  virtual byte_buffer &get_buffer()
  {
    return buffer;
  }

  Endian get_endian()
  {
    return buffer.endian;
  }

  void set_from_buffer(const std::vector<byte_t> &data)
  {
    len_t size = data.size();
    buffer.resize(size);
    memcpy(buffer.data(), data.data(), size);
  }

protected:
  len_t rpos = 0;
  byte_buffer buffer;
};
