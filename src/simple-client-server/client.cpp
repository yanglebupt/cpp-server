#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif
#include "net_common/net_client.hpp"
#include "common.cpp"
#include "json/json.h"
#include <iostream>
#include <iterator>
#include <regex>

Json::FastWriter json_writer = Json::FastWriter();

class CustomClient : public net::client_interface<custom_header>
{
public:
  CustomClient() : net::client_interface<custom_header>() {};
  CustomClient(int max_retries) : net::client_interface<custom_header>(max_retries) {};

  void PingServer()
  {
    net::message<custom_header> msg;
    msg.header.id = CustomMsgType::ServerPing;
    // 发送报文的时间
    msg << std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
    Send(std::move(msg));
  }

  void MessageAll(const Json::Value &json)
  {
    net::message<custom_header> msg;
    msg.header.id = CustomMsgType::MessageAll;
    msg << json_writer.write(json);
    Send(std::move(msg));
  }

protected:
  virtual void OnDisConnect() override
  {
    warn("Disconnect");
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

  CustomClient &c = (*new CustomClient(5));
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
    {
      delete &c;
      // exit_flag = true;
      // c.Close();
    }
  };

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
        double timeLast;
        msg >> timeLast;
        double timeNow = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
        info("Server ping: %5f", (timeNow - timeLast));
        break;
      }
      case CustomMsgType::ServerMessage:
      {
        uint32_t clientId;
        std::string json_str;
        msg >> clientId;
        msg >> json_str;
        info("Hello from [%d], %s", clientId, json_str.c_str());
      }
      }
    }
  }

  return 0;
}