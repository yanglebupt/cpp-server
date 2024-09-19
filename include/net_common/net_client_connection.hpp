#pragma once

#include "net_connection.hpp"
#include <thread>

namespace net
{
  template <typename T>
  class client_interface;

  template <typename T>
  class client_connection : public connection<T, client_connection<T>>
  {
  private:
    client_interface<T> *client;
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
            OnError(error_code::read_validation_error);
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
            OnError(error_code::write_validation_error);
          } });
    };

    void OnError(error_code ecode) override
    {
      client->DisConnectServer();
    };

    owned_message<T, client_connection<T>> PackMessage(const std::shared_ptr<message<T>> msg) override
    {
      owned_message<T, client_connection<T>> packed;
      packed.msg = std::move(*msg);
      // 返回值优化，可能不会拷贝
      return packed;
    };

  public:
    client_connection(client_interface<T> *client, asio::io_context &ctx, asio::ip::tcp::socket socket, tsqueue<owned_message<T, client_connection<T>>> &qIn) : connection<T, client_connection<T>>(ctx, std::move(socket), qIn), client(client)
    {
      this->owner = owner_type::client;
    };

    void ConnectToServer(const asio::ip::tcp::resolver::results_type &endpoints, int max_retries = 0, int retry_wait_ms = 0)
    {
      asio::async_connect(this->socket, endpoints, [this, max_retries, retry_wait_ms, &endpoints](std::error_code ec, asio::ip::tcp::endpoint endpoint)
                          {
            if (!ec) {
              // 接收服务端的验证码，计算响应码，并返回给服务端，就可以读取 Header 了
              ReadValidation();
            } else {
              // 这里可以考虑是否重新连接.....
              if (max_retries > 0){
                if (retry_wait_ms > 0)
                  std::this_thread::sleep_for(std::chrono::milliseconds(retry_wait_ms));
                std::cout << "Connected ReTry....." << std::endl;
                ConnectToServer(endpoints, max_retries - 1, retry_wait_ms);
              }
              else {
                std::cout << "Connected Failed" << std::endl;
                OnError(error_code::connection_error);
              }
            } });
    };
  };
}