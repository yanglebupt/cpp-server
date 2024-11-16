#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#endif
#include "net_common/net_server.hpp"
#include "common.cpp"
#include <iostream>

class CustomServer : public net::server_interface<custom_header>
{
public:
  CustomServer(uint16_t port) : net::server_interface<custom_header>(port) {}

protected:
  virtual bool ShouldAcceptClient(std::shared_ptr<net::server_connection<custom_header>> client) override
  {
    return true;
  }

  virtual void OnClientDisConnect(std::shared_ptr<net::server_connection<custom_header>> client) override
  {
    warn("Removing client [%d], Remain client count: %d", client->GetID(), ClientCount());
  }

  virtual void OnMessage(std::shared_ptr<net::server_connection<custom_header>> client, net::message<custom_header> &msg) override
  {
    switch (msg.header.id)
    {
    case CustomMsgType::ServerPing:
    {
      info("[%d] Server ping", client->GetID());
      client->Send(msg);
      break;
    }
    case CustomMsgType::MessageAll:
    {
      info("[%d] MessageAll", client->GetID());
      net::message<custom_header> back_msg;
      back_msg.header.id = CustomMsgType::ServerMessage;
      back_msg << client->GetID();
      back_msg << msg.get_body_buffer();
      SendAll(back_msg, client->GetID());
      break;
    }
    }
  }
};

int main()
{
  CustomServer &server = (*new CustomServer(5050));

  std::thread t([&server]()
                {
                  bool exit_flag = false;
                  CMDInputListener cmd_input_listner;
                  auto line_callback = [&](const std::string &line)
                  {
                    if (line == "exit")
                    {
                      delete &server;
                    }
                  };
                  while (!exit_flag)
                  {
                    cmd_input_listner(line_callback, false);
                  } });

  server.Start();

  t.join();

  return 0;
}