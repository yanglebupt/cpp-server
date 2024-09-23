#pragma once

#include "http_middleware.hpp"

namespace net::nhttp
{
  // router 本身也是一种中间件集合
  class http_router : public http_middleware_collection
  {
  public:
    virtual ~http_router() {}
    void Get(const PathArgument &path, RequestHandler route)
    {
      auto &mdles = middleware_map[compose_match_path(path, verb::get)];
      mdles.emplace_back(RequestHandlerConfig{.exact = true, .is_route = true}, route);
    };

    void Get(const PathArgument &path, RequestHandler middleware, RequestHandler route, bool exact = false)
    {
      auto &mdles = middleware_map[compose_match_path(path, verb::get)];
      mdles.emplace_back(RequestHandlerConfig{.exact = exact, .is_route = true}, middleware);
      mdles.emplace_back(RequestHandlerConfig{.exact = true, .is_route = true}, route);
    };

    void Post(const PathArgument &path, RequestHandler route)
    {
      auto &mdles = middleware_map[compose_match_path(path, verb::post)];
      mdles.emplace_back(RequestHandlerConfig{.exact = true, .is_route = true}, route);
    };

    void Post(const PathArgument &path, RequestHandler middleware, RequestHandler route, bool exact = false)
    {
      auto &mdles = middleware_map[compose_match_path(path, verb::post)];
      mdles.emplace_back(RequestHandlerConfig{.exact = exact, .is_route = true}, middleware);
      mdles.emplace_back(RequestHandlerConfig{.exact = true, .is_route = true}, route);
    };
  };
}