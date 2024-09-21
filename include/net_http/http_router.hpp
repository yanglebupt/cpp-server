#pragma once

#include "middleware.hpp"

namespace net::nhttp
{
  // router 本身也是一种中间件集合
  class router : public middleware_collection
  {
  public:
    void Get(const PathArgument &path, RequestHandler handler)
    {
      middleware_map[path].emplace_back(handler);
    };
  };
}