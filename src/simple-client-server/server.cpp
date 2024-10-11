#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#endif
#include "net_common/net_server.hpp"
#include "common.cpp"
#include <iostream>

class CustomServer : public net::server_interface<CustomMsgType>
{
public:
  CustomServer(uint16_t port) : net::server_interface<CustomMsgType>(port) {}

protected:
  virtual bool ShouldAcceptClient(std::shared_ptr<net::server_connection<CustomMsgType>> client) override
  {
    return true;
  }

  virtual void OnClientDisConnect(std::shared_ptr<net::server_connection<CustomMsgType>> client) override
  {
    std::cout << "Removing client [" << client->GetID() << "], Remain client count: " << ClientCount() << std::endl;
  }

  virtual void OnMessage(std::shared_ptr<net::server_connection<CustomMsgType>> client, net::message<CustomMsgType> &msg) override
  {
    switch (msg.header.id)
    {
    case CustomMsgType::ServerPing:
    {
      std::cout << "[" << client->GetID() << "]" << "Server ping" << std::endl;
      client->Send(msg);
      break;
    }
    case CustomMsgType::MessageAll:
    {
      std::cout << "[" << client->GetID() << "]" << "MessageAll" << std::endl;
      net::message<CustomMsgType> back_msg;
      back_msg.header.id = CustomMsgType::ServerMessage;
      back_msg << msg;
      back_msg << client->GetID();
      SendMessageAllClients(back_msg, client);
      break;
    }
    }
  }
};

int main()
{
  CustomServer server(5050);
  server.Start();

  return 0;
}