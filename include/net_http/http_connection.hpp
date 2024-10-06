#pragma once

#include "http_typedef.hpp"
#include "../utils/owned_message_interface.hpp"
#include "../utils/tsqueue.hpp"
#include <iostream>
#include <memory>

namespace net::nhttp
{
  class http_connection : public std::enable_shared_from_this<http_connection>
  {
  public:
    http_connection(tcp::socket socket, uint8_t keep_alive_timeout, uint8_t max_connection_count, tsqueue<owned_message_interface<Request, http_connection>> &qIn) : socket(std::move(socket)),
                                                                                                                                                                     keep_alive_timeout(keep_alive_timeout), max_connection_count(max_connection_count), qIn(qIn) {}

    ~http_connection()
    {
      std::cout << remote_endpoint() << " Connection Destroyed!" << std::endl;
    }

    void ConnectToClient()
    {
      ReadRequest();
      // 短连接时间到了，不能立刻关闭 socket，需要等待接收和发送完毕后才关闭
      deadline.async_wait([self = shared_from_this()](std::error_code ec)
                          {
                            std::cout << self->remote_endpoint() << " Connection Timeout!" << std::endl;
                            self->timeout = true;
                            // 那如果是在 async_write 过程中 timeout 了，需要等待 async_write 回调中进行取消
                            if (self->sending) return; 
                            self->Cancel(); });
    }

    void Cancel()
    {
      // 不允许继续接受和发送消息
      socket.shutdown(tcp::socket::shutdown_both);
      // 取消当前 socket 注册的异步任务，例如 async_read
      socket.cancel();
    }

    void ReadRequest()
    {
      // 接收数据的 buffer
      std::shared_ptr<beast::flat_buffer> buffer = std::make_shared<beast::flat_buffer>(1024 * 8);
      std::shared_ptr<Request> req = std::make_shared<Request>();
      http::async_read(socket, *buffer, *req, [self = shared_from_this(), req, buffer](std::error_code ec, size_t length)
                       {  if (ec) return;
                          self->c_count++;
                          bool need_keep_alive = self->NeedKeepAlive(*req);
                          owned_message_interface<Request, http_connection> msg;
                          msg.remote = self->shared_from_this();
                          msg.msg = std::move(*req);
                          self->qIn.emplace_back(msg);
                          if (need_keep_alive)
                            self->ReadRequest(); });
    }

    void WriteResponse(std::shared_ptr<Response> res, bool keep_alive)
    {
      // 写入响应
      res->set(http::field::server, "Beast");
      res->content_length(res->body().size());
      res->keep_alive(keep_alive);
      if (keep_alive)
        res->set(http::field::keep_alive, "timeout=" + std::to_string(keep_alive_timeout) + ", max=" + std::to_string(max_connection_count));

      sending = true;
      http::async_write(socket, *res, [self = shared_from_this(), res, keep_alive](std::error_code ec, size_t length)
                        {
                          self->sending = false;
                          if (self->timeout) self->Cancel();
                          else {
                            // 短链接的话，定时器不要再等待了，cancel 会以 asio::error::operation_aborted 触发回调
                            if (!keep_alive)
                              self->deadline.cancel();
                          } });
    }

    bool NeedClose() { return timeout || c_count >= max_connection_count; }

    bool NeedKeepAlive(Request &req) { return NeedClose() ? false : (req.keep_alive() || (req.find(http::field::connection) != req.end() && req.at(http::field::connection) == "keep-alive")); }

    tcp::endpoint remote_endpoint() { return socket.remote_endpoint(); }

  protected:
    uint8_t keep_alive_timeout;
    uint8_t max_connection_count;
    uint8_t c_count = 0;
    bool timeout = false;
    tcp::socket socket;
    // 定时器，超时检测，http 默认是短连接，超时直接断开连接
    asio::steady_timer deadline{socket.get_executor(), std::chrono::seconds(keep_alive_timeout)};
    tsqueue<owned_message_interface<Request, http_connection>> &qIn;

    bool sending = false;
  };
}
