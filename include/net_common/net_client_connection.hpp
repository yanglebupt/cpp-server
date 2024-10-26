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
    bool accepted = false;
    bool validation_ok = false;

    void ReadAccepted()
    {
      asio::async_read(this->socket, asio::buffer(&accepted, sizeof(bool)), [self = share()](std::error_code ec, std::size_t length)
                       {
        if (!ec) {
          if (self->accepted)
          {
            ok("Server accepted");
            // 开始验证
            self->ReadValidation();
          }
          else
          {
            warn("Server not accepted!");
            self->client->ConnectionFailed(error_code::bad_accepted_error);
          }
        } else {
          err("Read Accepted Failed");
          self->OnError(error_code::read_accepted_error);
        } });
    };

    void ReadValidationResult()
    {
      asio::async_read(this->socket, asio::buffer(&validation_ok, sizeof(bool)), [self = share()](std::error_code ec, std::size_t length)
                       {
        if (!ec) {
          if (self->validation_ok)
          {
            ok("Server validated");
            self->client->Connected();
            self->ReadHeader();
          }
          else
          {
            warn("Server validation failed!");
            self->client->ConnectionFailed(error_code::bad_validation_error);
          }
        } else {
          err("Read Validation Result Failed");
          self->OnError(error_code::read_validation_res_error);
        } });
    };

    void ReadValidation()
    {
      // 继承的 protected 属性，都要加上 this，不然编译不过，特别是匿名函数里面
      asio::async_read(this->socket, asio::buffer(&validation, sizeof(uint64_t)), [self = share()](std::error_code ec, std::size_t length)
                       {
          if (!ec) {
            self->response = self->scramble(self->validation);
            self->WriteValidation();
          } else {
            err("Read Validation Failed");
            self->OnError(error_code::read_validation_error);
          } });
    };

    void WriteValidation()
    {
      asio::async_write(this->socket, asio::buffer(&response, sizeof(uint64_t)), [self = share()](std::error_code ec, std::size_t length)
                        {
          if (!ec) {
            self->ReadValidationResult();
          } else {
            err("Write Validation Failed");
            self->OnError(error_code::write_validation_error);
          } });
    };

    void OnError(error_code ecode) override
    {
      client->OnError(ecode);
      client->Stop();
    };

    owned_message<T, client_connection<T>> PackMessage(const std::shared_ptr<message<T>> msg) override
    {
      owned_message<T, client_connection<T>> packed;
      packed.msg = std::move(*msg);
      // 返回值优化，可能不会拷贝
      return packed;
    };

    std::shared_ptr<client_connection<T>> share()
    {
      return std::dynamic_pointer_cast<client_connection<T>>(this->shared_from_this());
    }

  public:
    client_connection(client_interface<T> *client, asio::ip::tcp::socket socket, tsqueue<owned_message<T, client_connection<T>>> &qIn) : connection<T, client_connection<T>>(std::move(socket), qIn), client(client) {};

    void ConnectToServer(const asio::ip::tcp::resolver::results_type &endpoints, int max_retries = 0, int retry_wait_ms = 0)
    {
      asio::async_connect(this->socket, endpoints, [self = share(), max_retries, retry_wait_ms, &endpoints](std::error_code ec, asio::ip::tcp::endpoint endpoint)
                          {
            if (!ec) {
              // 接收服务端的验证码，计算响应码，并返回给服务端，就可以读取 Header 了
              self->ReadAccepted();
            } else {
              // 这里可以考虑是否重新连接.....
              if (max_retries > 0){
                if (retry_wait_ms > 0)
                  std::this_thread::sleep_for(std::chrono::milliseconds(retry_wait_ms));
                warn("Connected ReTry.....");
                self->ConnectToServer(endpoints, max_retries - 1, retry_wait_ms);
              }
              else {
                err("Connected Failed");
                self->client->ConnectionFailed(error_code::connection_error);
              }
            } });
    };
  };
}