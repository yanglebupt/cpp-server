#pragma once

#include "net_connection.hpp"

namespace net
{
  template <typename T>
  class server_interface;

  template <typename T>
  class server_connection : public connection<T, server_connection<T>>
  {
  private:
    server_interface<T> *server;

  private:
    uint64_t validation = 0;
    uint64_t exceptResponseValidation = 0;
    uint64_t response = 0;

    void UpdateValidation()
    {
      // 服务器产生
      validation = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
      exceptResponseValidation = this->scramble(validation);
      response = 0;
    }

    void ReadValidation()
    {
      std::cout << "Read" << std::endl;
      asio::read(this->socket, asio::buffer(&response, sizeof(uint64_t)));
      std::cout << "Read Done" << response << std::endl;

      // asio::async_read(this->socket, asio::buffer(&response, sizeof(uint64_t)), [this](std::error_code ec, std::size_t length)
      //                  {
      //   if (!ec) {
      //     if (response == exceptResponseValidation)
      //     {
      //       std::cout << "[" << this->id << "] Validation OK" << std::endl;
      //       // server->OnClientValidated(this->share());
      //       this->ReadHeader();
      //     }
      //     else
      //     {
      //       std::cout << "[" << this->id << "] Validation Failed, Close" << std::endl;
      //       this->socket.close();
      //     }
      //   } else {
      //     std::cout << "[" << this->id << "] Read Validation Failed" << std::endl;
      //     this->socket.close();
      //   } });
    }

    void WriteValidation()
    {
      // std::cout << "Write Validation" << validation << std::endl;
      // try
      // {
      //   asio::write(this->socket, asio::buffer(&validation, sizeof(uint64_t)));
      //   std::cout << "Write Done" << std::endl;
      // }
      // catch (const std::exception &e)
      // {
      //   std::cerr << e.what() << '\n';
      // }
      // ReadValidation();

      asio::async_write(this->socket, asio::buffer(&validation, sizeof(uint64_t)), [this](std::error_code ec, std::size_t length)
                        {
        std::cout << "Write Validation Done" << ec.message() <<validation << std::endl;
        if (!ec) {
          std::cout << "Write Validation SUC" << validation << std::endl;
          // 等待客户端返回响应码，内部校验通过，开始读取 Header
          // ReadValidation();
        } else {
          std::cout << "[" << this->id << "] Write Validation Failed" << std::endl;
          this->socket.close();
        } });
    }

    owned_message<T, server_connection<T>> PackMessage(message<T> &message) override
    {
      owned_message<T, server_connection<T>> packed;
      // packed.remote = this->share();
      packed.msg = std::move(message);
      // 返回值优化，可能不会拷贝
      return std::move(packed);
    };

    // std::shared_ptr<server_connection<T>> share()
    // {
    //   return std::dynamic_pointer_cast<server_connection<T>>(this->shared_from_this());
    // }

  public:
    server_connection(server_interface<T> *server, asio::io_context &ctx, asio::ip::tcp::socket socket, tsqueue<owned_message<T, server_connection<T>>> &qIn) : connection<T, server_connection<T>>(ctx, std::move(socket), qIn), server(server)
    {
      // 连接的验证码，可以一段时间更新一次，不要每个请求都去更新，太耗时了
      UpdateValidation();
      this->owner = connection<T, server_connection<T>>::owner_type::server;
    };

    void ConnectToClient(uint32_t serverClientID)
    {
      if (this->socket.is_open())
      {
        this->id = serverClientID;
        // 向客户端发送验证码
        WriteValidation();
      }
    }
  };
}