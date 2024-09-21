#pragma once

#include "http_typedef.hpp"
#include <map>
#include <deque>

class middleware_collection : public RequestHandler
{
protected:
  // 不同路径下的中间件队列 ---> 组成中间件集合
  // 后面将会进一步支持多路径/多中间件注册，并且支持限制请求方法
  std::map<PathArgument, std::deque<RequestHandler>> middleware_map;

public:
  // 注册中间件/或者中间件集合
  // 注意这些方法注册的中间件是有顺序的，先调用方法注册的中间件先执行，因此需要使用队列
  void Use(RequestHandler handler)
  {
    middleware_map[ROOT_PATH].emplace_back(handler);
  };

  void Use(const PathArgument &path, RequestHandler handler)
  {
    middleware_map[path].emplace_back(handler);
  };
};
