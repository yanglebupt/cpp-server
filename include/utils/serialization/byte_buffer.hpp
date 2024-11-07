#pragma once
#include <ostream>
#include <vector>
#include <iomanip>
#include <stdint.h>
#include <string>
#include <cstring>
#include <type_traits>
#include <algorithm>
#include <fstream>
#include <io.h>
#include <iostream>

typedef uint8_t byte_t;
typedef int32_t len_t;

template <typename T>
inline constexpr bool is_primitive_type_v = std::is_same_v<T, bool> ||
                                            std::is_same_v<T, char> || std::is_same_v<T, uint8_t> ||
                                            std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t> ||
                                            std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t> ||
                                            std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t> ||
                                            std::is_same_v<T, float> || std::is_same_v<T, double>;

enum Endian
{
  BE,
  LE,
};

// 获取当前设备的字节序
Endian get_device_endian()
{
  int i = 1;
  char *i_ptr = (char *)(&i);
  return *i_ptr == 1 ? Endian::LE : Endian::BE;
}

static Endian device_endian = get_device_endian();

class byte_buffer : public std::vector<byte_t>
{
public:
  friend class data_stream;
  byte_buffer() : endian(device_endian) {}
  byte_buffer(Endian endian) : endian(endian) {}

  // 读取基本数据
  template <typename T>
  len_t read(T &value, len_t offset = 0)
  {
    if constexpr (is_primitive_type_v<T>)
    {
      len_t len = sizeof(T);
      std::memcpy(&value, this->data() + offset, len);
      if (endian != device_endian)
      {
        byte_t *data = (byte_t *)&value;
        std::reverse(data, data + len);
      }
      return len;
    }
    return -1;
  };

  // 写入基本数据类型
  template <typename T>
  void write(T data, len_t offset = -1)
  {
    if constexpr (is_primitive_type_v<T>)
      write((byte_t *)&data, offset, sizeof(T));
  };

  // 打印
  friend std::ostream &operator<<(std::ostream &out, const byte_buffer &buffer)
  {
    int print_max_bytes = buffer.print_max_bytes, n = buffer.size();
    out << "<Total " << n << " Bytes ";
    std::ios::fmtflags original_flags = out.flags();
    for (size_t i = 0; i < print_max_bytes; i++)
    {
      if (i >= n)
        break;
      out << std::setw(2) << std::setfill('0') << std::hex << (uint16_t)buffer[i];
      out.flags(original_flags);
      if (!(i == n - 1 || i == print_max_bytes - 1))
        out << " ";
    }
    if (print_max_bytes < n)
      out << " ... More " << (n - print_max_bytes) << " Bytes";
    out << ">";
    return out;
  }

  // 保存
  void save(const char *filename)
  {
    // 不存在文件夹，需要先创建，这里先省略
    std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
    if (!ofs)
      error_msg("create file error, please make directory first!");
    ofs.write((char *)data(), size());
    ofs.flush();
    ofs.close();
  };

  // 加载
  void load(const char *filename)
  {
    if (access(filename, F_OK) == -1)
      error_msg("file not found!");
    // 在文件末尾打开文件，这样后面才可以获取文件大小
    std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
    if (!ifs)
      error_msg("cann't open file!");
    len_t size = ifs.tellg();
    resize(size);
    // 回到起始位置开始读取
    ifs.seekg(0, std::ios::beg);
    ifs.read((char *)data(), size);
    ifs.close();
  };

protected:
  Endian endian;
  int print_max_bytes = 30;

  // 偏移写入多少个字节
  void write(byte_t *data, len_t offset, len_t len, bool need_check_endian = true)
  {
    // 翻转
    if (need_check_endian && endian != device_endian)
      std::reverse(data, data + len);
    len_t start = offset < 0 ? this->size() : offset;
    len_t end = start + len;
    if (end > this->size())
      this->resize(end);
    std::memcpy(this->data() + start, data, len);
  };

  void error_msg(const std::string &msg)
  {
    std::cout << msg << std::endl;
    throw std::runtime_error(msg);
  }
};
