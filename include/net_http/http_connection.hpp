#pragma once

#include "http_typedef.hpp"
#include "../utils/functions.hpp"
#include <iostream>

namespace net::nhttp
{
  class http_connection : public std::enable_shared_from_this<http_connection>
  {
  public:
    http_connection(tcp::socket socket) : socket(std::move(socket)) {}

    ~http_connection()
    {
      std::cout << req.method_string() << " " << req.Get<std::string>("path") << " Connection Destroyed!" << std::endl;
    }

    void ReadRequest(RequestHandlerCallback _Callback)
    {
      http::async_read(socket, buffer, req, [self = shared_from_this(), _Callback](std::error_code ec, size_t length)
                       { if(!ec) self->HandleRequest(_Callback); });

      // 短连接时间到了，关闭 socket
      deadline.async_wait([self = shared_from_this()](std::error_code ec)
                          { std::cout << self->req.method_string() << " " << self->req.Get<std::string>("path") << " Timeout: " << ec << std::endl;
                            if(!ec) self->socket.close(); });
    }

    void HandleRequest(RequestHandlerCallback _Callback)
    {
      // 设置响应头
      res.version(req.version());
      res.keep_alive(false);

      // 解析地址，得到 query，需要为 req 添加新属性
      auto [url, search] = SplitURLSearch(req.target());
      req.Add("path", url);
      req.Add("search", search);
      req.Add("query", URLSearchParams(search));

      _Callback(req, res);

      // 写入响应
      res.set(http::field::server, "Beast");
      res.content_length(res.body().size());
      http::async_write(socket, res, [self = shared_from_this()](std::error_code ec, size_t length)
                        {
                          self->socket.shutdown(tcp::socket::shutdown_send);
                          // 定时器不要再等待了，cancel 会以 asio::error::operation_aborted 触发回调
                          self->deadline.cancel(); });
    }

  protected:
    tcp::socket socket;
    // 接收数据的 buffer
    beast::flat_buffer buffer{1024 * 8};

    // 后面会将数据 buffer 解析到 req/res 中
    Request req;
    Response res;

    // 定时器，超时检测，http 默认是短连接，超时直接断开连接
    asio::steady_timer deadline{
        socket.get_executor(), std::chrono::seconds(60)};
  };
}
