#include <iostream>
#include "net_common/net_message.hpp"

bool isLittleEndian()
{
  int i = 1;
  char *i_ptr = reinterpret_cast<char *>(&i);
  return *i_ptr == 1;
};

enum class SystemMessage : uint32_t
{
  FireBullet,
  MovePlayer
};

void rw()
{
  net::message<SystemMessage> msg;
  msg.header.id = SystemMessage::MovePlayer;

  std::cout << msg << std::endl;

  int a = 1;
  bool b = false;
  float c = 3.1415f;

  struct
  {
    float x, y;
  } d[5];

  for (size_t i = 0; i < 5; i++)
  {
    d[i].x = (i + 1) * 10;
    d[i].y = (i + 1) * 10 + 5;
  }

  msg << a << b << c << d;

  std::cout << msg << std::endl;
  std::cout << sizeof(int) + sizeof(bool) + sizeof(float) + sizeof(float) * 10 << std::endl;

  a = 99;
  b = true;
  c = 99.f;

  msg >> d >> c >> b >> a;

  std::cout << msg << std::endl;
  std::cout << "a: " << a << ", b: " << b << ", c: " << c << std::endl;

  for (size_t i = 0; i < 5; i++)
  {
    std::cout << "d[" << i << +"]: " << d[i].x << "," << d[i].y << std::endl;
  }
}

void Send(const net::message<SystemMessage> &msg)
{
  auto func = [msg = std::move(const_cast<net::message<SystemMessage> &>(msg))]() mutable
  {
    uint32_t id;
    msg >> id;
    std::cout << "id: " << id << std::endl;
  };
  func();
}

int main()
{
  net::message<SystemMessage> msg;
  msg.header.id = SystemMessage::MovePlayer;

  Json::Value s;
  s["name"] = "mike";
  s["age"] = 10;

  // char s[] = "hello";
  msg << s;
  msg << 1001;

  net::message<SystemMessage> msg2;
  msg2.header.id = SystemMessage::MovePlayer;
  msg2 << msg;

  net::message<SystemMessage> msg3;
  msg3.header.id = SystemMessage::MovePlayer;

  msg2 >> msg3;

  int i;
  // std::string s2;
  Json::Value s2;
  msg3 >> i;
  msg3 >> s2;

  std::cout << i << "," << s2.toStyledString() << std::endl;

  std::cout << msg.size() << "," << msg2.size() << std::endl;

  // Send(msg);

  return 0;
}