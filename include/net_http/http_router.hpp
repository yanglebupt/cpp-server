#pragma once

#include "http_middleware.hpp"

namespace net::nhttp
{
  // router 本身也是一种中间件集合
  class router : public middleware_collection
  {
  public:
    void Get(const PathArgument &path, RequestHandler route)
    {
      auto &mdles = middleware_map[this->compose_match_path(path, verb::get)];
      mdles.emplace_back(true, route);
    };

    void Get(const PathArgument &path, RequestHandler middleware, RequestHandler route, bool exact = false)
    {
      auto &mdles = middleware_map[this->compose_match_path(path, verb::get)];
      mdles.emplace_back(exact, middleware);
      mdles.emplace_back(true, route);
    };

    void Post(const PathArgument &path, RequestHandler route)
    {
      auto &mdles = middleware_map[this->compose_match_path(path, verb::post)];
      mdles.emplace_back(true, route);
    };

    void Post(const PathArgument &path, RequestHandler middleware, RequestHandler route, bool exact = false)
    {
      auto &mdles = middleware_map[this->compose_match_path(path, verb::post)];
      mdles.emplace_back(exact, middleware);
      mdles.emplace_back(true, route);
    };

    /**
     * Any is same as Use middleware
     * @deprecated `Use` function
     */
    void Any(const PathArgument &path, RequestHandler route)
    {
      auto &mdles = middleware_map[this->compose_match_path(path)];
      mdles.emplace_back(true, route);
    };

    /**
     * @deprecated `Use` function
     */
    void Any(const PathArgument &path, RequestHandler middleware, RequestHandler route, bool exact = false)
    {
      auto &mdles = middleware_map[this->compose_match_path(path)];
      mdles.emplace_back(exact, middleware);
      mdles.emplace_back(true, route);
    };
  };
}