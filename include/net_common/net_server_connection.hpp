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
    uint64_t validation = 0;
    uint64_t exceptResponseValidation = 0;
    uint64_t response = 0;

    // 连接的验证码，可以一段时间更新一次，不要每个请求都去更新，太耗时了
    void UpdateValidation()
    {
      // 服务器产生
      validation = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
      exceptResponseValidation = this->scramble(validation);
      response = 0;
    }

    void ReadValidation()
    {
      asio::async_read(this->socket, asio::buffer(&response, sizeof(uint64_t)), [self = share()](std::error_code ec, std::size_t length)
                       {
        if (!ec) {
          self->WriteValidationResult(self->response == self->exceptResponseValidation);
        } else {
          err("[%d] Read Validation Failed", self->id);
          self->OnError(error_code::read_validation_error);
        } });
    }

    void WriteValidation()
    {
      asio::async_write(this->socket, asio::buffer(&validation, sizeof(uint64_t)), [self = share()](std::error_code ec, std::size_t length)
                        {
        if (!ec) {
          // 等待客户端返回响应码，内部校验通过，开始读取 Header
          self->ReadValidation();
        } else {
          err("[%d] Write Validation Failed", self->id);
          self->OnError(error_code::write_validation_error);
        } });
    }

    void WriteValidationResult(bool validation_ok)
    {
      asio::async_write(this->socket, asio::buffer(&validation_ok, sizeof(bool)), [self = share(), validation_ok](std::error_code ec, std::size_t length)
                        {
        if (!ec) {
          if (validation_ok)
          {
            ok("[%d] Validation OK", self->id);
            self->server->AddClient(self->share());
            self->server->OnClientValidated(self->share());
            self->ReadHeader();
          }
          else
          {
            warn("[%d] Validation Failed", self->id);
            self->OnError(error_code::bad_validation_error);
          }
        } else {
          err("[%d] Write Validation Result Failed", self->id);
          self->OnError(error_code::write_validation_res_error);
        } });
    };

    void OnError(error_code ecode) override
    {
      server->OnError(ecode);
      server->RemoveClient(this->id);
    };

    owned_message<T, server_connection<T>> PackMessage(const std::shared_ptr<message<T>> msg) override
    {
      owned_message<T, server_connection<T>> packed;
      packed.remote = this->share();
      packed.msg = std::move(*msg);
      // 返回值优化，可能不会拷贝
      return packed;
    };

    std::shared_ptr<server_connection<T>> share()
    {
      return std::dynamic_pointer_cast<server_connection<T>>(this->shared_from_this());
    }

  public:
    server_connection(server_interface<T> *server, asio::ip::tcp::socket socket, tsqueue<owned_message<T, server_connection<T>>> &qIn) : connection<T, server_connection<T>>(std::move(socket), qIn), server(server) {};

    void ConnectToClient(uint32_t serverClientID, bool accepted)
    {
      this->id = serverClientID;
      asio::async_write(this->socket, asio::buffer(&accepted, sizeof(bool)), [self = share(), accepted](std::error_code ec, std::size_t length)
                        {
        if (!ec) {
          if (accepted) {
            ok("[%d] Connection Approved", self->id);
            // 开始验证
            self->UpdateValidation();
            // 需要向客户端发送验证码
            self->WriteValidation();
          } else {
            warn("[%d] Connection Denied", self->id);
            self->OnError(error_code::bad_accepted_error);
          }
        } else {
          err("[%d] Write Accepted Failed", self->id);
          self->OnError(error_code::write_accepted_error);
        } });
    };
  };
}