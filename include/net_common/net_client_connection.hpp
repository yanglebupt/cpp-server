#pragma once

#include "net_connection.hpp"

namespace net
{
  template <typename T>
  class client_connection : public connection<T, client_connection<T>>
  {
  private:
    uint64_t validation = 0;
    uint64_t response = 0;

    void ReadValidation()
    {
      // 继承的 protected 属性，都要加上 this，不然编译不过，特别是匿名函数里面
      asio::async_read(this->socket, asio::buffer(&validation, sizeof(uint64_t)), [this](std::error_code ec, std::size_t length)
                       {
          if (!ec) {
            response = this->scramble(validation);
            WriteValidation();
          } else {
            std::cout << "Read Validation Failed" << std::endl;
            this->socket.close();
          } });
    };

    void WriteValidation()
    {
      asio::async_write(this->socket, asio::buffer(&response, sizeof(uint64_t)), [this](std::error_code ec, std::size_t length)
                        {
          if (!ec) {
            this->ReadHeader();
          } else {
            std::cout << "Write Validation Failed" << std::endl;
            this->socket.close();
          } });
    };

    owned_message<T, client_connection<T>> PackMessage(message<T> &message) override
    {
      owned_message<T, client_connection<T>> packed;
      packed.remote = nullptr;
      packed.msg = std::move(message);
      // 返回值优化，可能不会拷贝
      return std::move(packed);
    };

  public:
    client_connection(asio::io_context &ctx, asio::ip::tcp::socket socket, tsqueue<owned_message<T, client_connection<T>>> &qIn) : connection<T, client_connection<T>>(ctx, std::move(socket), qIn)
    {
      this->owner = connection<T, client_connection<T>>::owner_type::client;
    };

    void ConnectToServer(const asio::ip::tcp::resolver::results_type &endpoints)
    {
      asio::async_connect(this->socket, endpoints, [this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
                          {
            if(!ec){
              // 接收服务端的验证码，计算响应码，并返回给服务端，就可以读取 Header 了
              ReadValidation();
            } });
    };
  };
}