#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#endif
#include "net_common/net_server.hpp"
#include <iostream>

// client and server all need this
enum class CustomMsgType : uint32_t
{
  ServerAccept,
  ServerValidated,
  ServerDeny,
  ServerPing,
  MessageAll,
  ServerMessage,
};

class CustomServer : public net::server_interface<CustomMsgType>
{
public:
  CustomServer(uint16_t port) : net::server_interface<CustomMsgType>(port) {}

protected:
  virtual bool OnClientConnect(std::shared_ptr<net::server_connection<CustomMsgType>> client)
  {
    net::message<CustomMsgType> msg;
    msg.header.id = CustomMsgType::ServerAccept;
    client->Send(msg);
    return true;
  }

  virtual void OnClientValidated(std::shared_ptr<net::server_connection<CustomMsgType>> client)
  {
    net::message<CustomMsgType> msg;
    msg.header.id = CustomMsgType::ServerValidated;
    client->Send(msg);
  }

  virtual void OnClientDisConnect(std::shared_ptr<net::server_connection<CustomMsgType>> client)
  {
    std::cout << "Removing client [" << client->GetID() << "], Remain client count: " << ClientCount() << std::endl;
  }

  // virtual void OnMessage(std::shared_ptr<net::server_connection<CustomMsgType>> client, net::message<CustomMsgType> &msg)
  // {
  //   switch (msg.header.id)
  //   {
  //   case CustomMsgType::ServerPing:
  //   {
  //     std::cout << "[" << client->GetID() << "]" << "Server ping" << std::endl;
  //     client->Send(msg);
  //     break;
  //   }
  //   case CustomMsgType::MessageAll:
  //   {
  //     std::cout << "[" << client->GetID() << "]" << "MessageAll" << std::endl;
  //     net::message<CustomMsgType> msg;
  //     msg.header.id = CustomMsgType::ServerMessage;
  //     msg << client->GetID();
  //     SendMessageAllClients(msg, client);
  //     break;
  //   }
  //   }
  // }
};

int main()
{
  CustomServer server(5050);
  server.Start();
  // server.Join();

  while (true)
  {
    server.Update();
  }

  return 0;
}