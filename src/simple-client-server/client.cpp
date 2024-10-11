#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif
#include "net_common/net_client.hpp"
#include "common.cpp"
#include "json/json.h"
#include <iostream>
#include <conio.h>
#include <functional>
#include <iterator>
#include <regex>

class CustomClient : public net::client_interface<CustomMsgType>
{
public:
  CustomClient() : net::client_interface<CustomMsgType>() {};
  CustomClient(int max_retries) : net::client_interface<CustomMsgType>(max_retries) {};
  void PingServer()
  {
    net::message<CustomMsgType> msg;
    msg.header.id = CustomMsgType::ServerPing;
    // 发送报文的时间
    std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
    msg << timeNow;
    Send(std::move(msg));
  }

  void MessageAll(const Json::Value &json)
  {
    net::message<CustomMsgType> msg;
    msg.header.id = CustomMsgType::MessageAll;
    msg << json;
    Send(std::move(msg));
  }

  virtual void OnServerDisConnect() override
  {
    std::cout << "Server disconnect" << std::endl;
  }
};

class CMDInputListener
{
private:
  std::string line;

public:
  // 通过回调函数监听并处理每一行输入，回调不允许是异步函数
  void operator()(std::function<void(const std::string &)> _Line_Callback, bool hidden = false, std::function<void(char, const std::string &)> _Char_Callback = nullptr)
  {
    char ch;
    if (_kbhit())
    {                // 检查是否有输入
      ch = _getch(); // 获取输入字符
      if (ch == 13)
      {
        std::cout << std::endl;
        _Line_Callback(line);
        line.clear();
      }
      else
      {
        if (!hidden)
          std::cout << ch;
        if (_Char_Callback != nullptr)
          _Char_Callback(ch, line);
        line += ch;
      }
    }
  }
};

/*
  用 delim 指定的正则表达式将字符串 in 分割，返回分割后的字符串数组
*/
std::vector<std::string> s_split(const std::string &in, const std::string &delim)
{
  std::regex re{delim};
  // 构造函数,完成字符串分割
  return std::vector<std::string>{
      std::sregex_token_iterator(in.begin(), in.end(), re, -1),
      std::sregex_token_iterator()};
}

int main()
{
  std::cout << "Press 1: Ping Server" << std::endl;
  std::cout << "Press 2: Send Json Message to All Other Clients" << std::endl;
  std::cout << "Press 3: Exit" << std::endl;

  CustomClient c(5);
  c.Connect("127.0.0.1", 5050);

  bool exit_flag = false;

  CMDInputListener cmd_input_listner;
  auto line_callback = [&](const std::string &line)
  {
    char command = line[0];
    if (command == '1')
      c.PingServer();
    if (command == '2')
    {
      std::vector<std::string> splits = s_split(line.substr(1), ",");
      Json::Value root;
      root["name"] = splits[0];
      root["age"] = std::stoi(splits[1]);
      c.MessageAll(root);
    }
    if (command == '3')
      exit_flag = true; };

  auto char_callback = [&](char ch, const std::string &line)
  {
    if (ch == '2' && line.length() == 0) // 第一个字符是 2，代表后面要输入 JSON 了
    {
      std::cout << " Json Input -> name: "; // 第一个字段
    }
    else if (ch == ',' && line[0] == '2') // 第二个字段
    {
      std::cout << " age: ";
    }
  };

  while (!exit_flag)
  {
    cmd_input_listner(line_callback, false, char_callback);

    if (!c.InComing().empty())
    {
      // 将亡值 move 延长生命周期
      auto msg = c.InComing().pop_front().msg;
      switch (msg.header.id)
      {
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
        std::string json_str;
        msg >> clientId;
        msg >> json_str;
        std::cout << "Hello from [" << clientId << "], " << json_str << std::endl;
      }
      }
    }
  }

  return 0;
}