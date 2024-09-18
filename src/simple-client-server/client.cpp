#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif
#include "net_common/net_client.hpp"
#include "common.cpp"
#include <iostream>
#include <conio.h>
#include <functional>

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

class CMDInputListener
{
private:
  std::string line;

public:
  // 通过回调函数监听并处理每一行输入，回调不允许是异步函数
  void operator()(std::function<void(const std::string &)> _Callback, bool hidden = false)
  {
    char ch;
    if (_kbhit())
    {                // 检查是否有输入
      ch = _getch(); // 获取输入字符
      if (ch == 13)
      {
        std::cout << std::endl;
        _Callback(line);
        line.clear();
      }
      else
      {
        if (!hidden)
          std::cout << ch;
        line += ch;
      }
    }
  }
};

int main()
{
  CustomClient c;
  c.Connect("127.0.0.1", 5050);

  CMDInputListener cmd_input_listner;

  std::cout << "Press 1: Ping Server" << std::endl;
  std::cout << "Press 2: Send Hello to All Other Clients" << std::endl;
  std::cout << "Press 3: Exit" << std::endl;

  bool exit_flag = false;

  while (!exit_flag)
  {
    cmd_input_listner([&](const std::string &line)
                      {
                        char command = line[0];
                        if (command == '1')
                          c.PingServer();
                        if (command == '2')
                          c.MessageAll();
                        if (command == '3')
                          exit_flag = true; });

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

  return 0;
}