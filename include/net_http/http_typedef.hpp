#pragma once

#include "http_ns.hpp"
#include "../utils/properties.hpp"
#include <functional>

namespace net::nhttp
{
#define ROOT_PATH "/"
#define ANY_METHOD "ANY"
#define MATCH_PROTO ":"

  struct Request : public http::request<http::dynamic_body>, public Properties
  {
  };

  struct Response : public http::response<http::dynamic_body>, public Properties
  {
  };

  // 后面会支持正则路径匹配
  typedef std::string PathArgument;
  typedef std::function<void(void)> NextFunction;
  typedef std::function<void(Request &, Response &)> RequestHandlerCallback;
  // 中间件就是一个函数
  typedef std::function<void(Request &, Response &, NextFunction next)> RequestHandler;

  struct RequestHandlerWrapper
  {
    // 可以添加一些匹配规则
    bool exact;
    RequestHandler handler;
    RequestHandlerWrapper(bool exact, RequestHandler handler) : exact(exact), handler(handler) {};
  };

  /**
   * 默认状态码 200，默认 Content-Type="text/plain"
   */
  void End(Response &res, const beast::string_view &body)
  {
    res.result(http::status::ok);
    res.set(http::field::content_type, "text/plain");
    beast::ostream(res.body()) << body;
  }
}