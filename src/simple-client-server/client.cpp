#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif
#include "net_common/net_client.hpp"
#include "common.cpp"
#include <iostream>

class CustomClient : public net::client_interface<CustomMsgType>
{
public:
  void PingServer()
  {
    net::message<CustomMsgType> msg;
    msg.header.id = CustomMsgType::ServerPing;

    // 发送报文的时间
    std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();

    msg << timeNow;
    Send(msg);
  }

  void MessageAll()
  {
    net::message<CustomMsgType> msg;
    msg.header.id = CustomMsgType::MessageAll;
    Send(msg);
  }
};

int main()
{
  CustomClient c;
  c.Connect("127.0.0.1", 5050);

  std::cout << "Press 1: Ping Server" << std::endl;
  std::cout << "Press 2: Send Hello to All Other Clients" << std::endl;
  std::cout << "Press 3: Exit" << std::endl;

  int command = 0;
  bool exit = false;

  std::thread t([&]()
                {while (!exit)
                {
                  std::cin >> command;
                } });

  while (!exit)
  {
    if (c.IsConnected())
    {
      if (command == 1)
        c.PingServer();
      if (command == 2)
        c.MessageAll();
      if (command == 3)
        exit = true;

      command = 0;

      if (!c.InComing().empty())
      {
        // 将亡值 move 延长生命周期
        auto msg = c.InComing().pop_front().msg;
        switch (msg.header.id)
        {
        case CustomMsgType::ServerAccept:
        {
          std::cout << "Server accepted" << std::endl;
          break;
        }
        case CustomMsgType::ServerValidated:
        {
          std::cout << "Server validated" << std::endl;
          break;
        }
        case CustomMsgType::ServerDeny:
        {
          std::cout << "Server denied" << std::endl;
          break;
        }
        case CustomMsgType::ServerPing:
        {
          std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
          std::chrono::system_clock::time_point timeLast;
          msg >> timeLast;
          std::cout << "Server ping: " << std::chrono::duration<double>(timeNow - timeLast).count() << std::endl;
          break;
        }
        case CustomMsgType::ServerMessage:
        {
          uint32_t clientId;
          msg >> clientId;
          std::cout << "Hello from [" << clientId << "]" << std::endl;
        }
        }
      }
    }
  }

  if (t.joinable())
    t.join();

  return 0;
}