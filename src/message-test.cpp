#include "net_common/net_message.hpp"
#include <vector>
#include <memory>
#include <thread>
template <typename T>
typename std::underlying_type<T>::type PrintEnum(T const value)
{
  return static_cast<typename std::underlying_type<T>::type>(value);
}

enum class Cmd : uint32_t
{
  Do,
  He,
};

struct cus_header : net::header
{
  SERIALIZE(size, m_c, m_e, m_g, cmd);
  char m_c;
  double m_e;
  int m_g;
  Cmd cmd;

  void
  print()
  {
    std::cout << size << "," << m_c << "," << m_e << "," << m_g << "," << PrintEnum(cmd) << std::endl;
  }
};

struct cus_body : public serializable
{
  SERIALIZE(y);
  float y;
  void print()
  {
    std::cout << y << "," << std::endl;
  }
};

struct A
{
  std::thread t;
  A()
  {
    t = std::thread([]() {

    });
    std::cout << "A con" << std::endl;
  }
  ~A()
  {
    std::cout << "A de" << std::endl;
  }
};

int main()
{
  A a;
  // net::message<cus_header> pack;
  // len_t header_size = pack.header.__size;
  // std::cout << header_size << std::endl;

  // pack.header.m_e = 10.2;
  // pack.header.m_g = 13;
  // pack.header.m_c = 'a';
  // cus_body body;
  // body.y = 20.3f;
  // pack = body;

  // // 接收到字节流
  // std::vector<byte_t> &buffer = pack.get_buffer();
  // net::message<cus_header> msg2;

  // std::vector<byte_t> header_data(header_size);
  // memcpy(header_data.data(), buffer.data(), header_size);
  // msg2.set_header_from_buffer(header_data);

  // msg2.header.print();

  // std::vector<byte_t> body_data(msg2.header.size);
  // memcpy(body_data.data(), buffer.data() + header_size, msg2.header.size);
  // msg2.set_body_from_buffer(body_data);

  // cus_body b;
  // msg2 >> b;

  // b.print();

  return 0;
}
