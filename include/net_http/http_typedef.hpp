#pragma once

#include "http_ns.hpp"
#include "../utils/properties.hpp"
#include <functional>
#include <variant>

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
  class http_middleware_collection;
  typedef std::function<void(Request &, Response &, NextFunction next)> http_middleware_func;
  typedef std::variant<http_middleware_collection *, http_middleware_func> RequestHandler;

  // 可以添加一些匹配规则
  struct RequestHandlerConfig
  {
    bool exact = false;
    bool is_route = false;
  };

  struct RequestHandlerWrapper
  {
    RequestHandlerConfig config;
    RequestHandler handler;
    RequestHandlerWrapper(RequestHandlerConfig config, RequestHandler handler) : config(config), handler(handler) {};
    ~RequestHandlerWrapper();

    http_middleware_collection *GetMiddlewareCollection()
    {
      http_middleware_collection **res = std::get_if<http_middleware_collection *>(&handler);
      return res == nullptr ? nullptr : *res;
    };

    http_middleware_func *GetMiddlewareFunc()
    {
      http_middleware_func *res = std::get_if<http_middleware_func>(&handler);
      return res;
    };
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