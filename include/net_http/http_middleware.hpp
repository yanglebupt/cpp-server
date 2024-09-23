#pragma once

#include "http_typedef.hpp"
#include "../utils/functions.hpp"
#include <map>
#include <deque>

namespace net::nhttp
{
  PathArgument compose_match_path(const PathArgument &path, verb method)
  {
    auto method_string_view = boost::beast::http::to_string(method);
    PathArgument match_path{method_string_view.data(), method_string_view.size()};
    match_path += MATCH_PROTO;
    match_path += path;
    return match_path;
  }

  PathArgument compose_match_path(const PathArgument &path)
  {
    PathArgument match_path{ANY_METHOD};
    match_path += MATCH_PROTO;
    match_path += path;
    return match_path;
  }

  class http_middleware_collection
  {
  protected:
    // 不同路径下的中间件队列 ---> 组成中间件集合
    std::map<PathArgument, std::deque<RequestHandlerWrapper>> middleware_map;

    /**
     * 中间件的匹配字符格式
     *
     * [<METHOD>::]<PATH>
     *
     * - METHOD = ANY 匹配所有方法
     * - PATH = /a 前缀匹配即可，匹配所有子路径，例如 /a/b /a/b/c
     *
     * 中间件执行优先级，ANY 先执行，匹配到的短路径先执行
     * 假设请求是 GET /a/b/c，那么匹配顺序是
     *
     * ANY::/  GET::/
     * ANY::/a  GET::/a
     * ANY::/a/b  GET::/a/b
     * ANY::/a/b/c  GET::/a/b/c (路由只会执行最后一个)
     *
     * 如果注册中间件时设置了 exact = true，那么同样中间件也只会执行最后一个
     * 注意 exact 对 ANY 不起作用，例如设置了 /a/b/c 的中间件（不指定 method 就是 ANY），并且 exact = true
     * 对于 GET /a/b/c 仍然会执行该中间件，但是不会执行 exact = true 的 ANY::/a/b 中间件（如果有的话）
     */
    std::deque<http_middleware_func> get_matched_paths(const PathArgument &path, const std::string &method, bool &has_route)
    {
      PathArgument try_any_match_prefix{ANY_METHOD};
      try_any_match_prefix += MATCH_PROTO;

      PathArgument try_method_match_prefix{method};
      try_method_match_prefix += MATCH_PROTO;

      std::deque<http_middleware_func> middlewares;

      std::vector<std::string> subpaths = SplitString(path, "/");
      for (size_t i = 0, n = subpaths.size(); i < n; i++)
      {
        const std::string &subpath = subpaths[i];
        handle_prefix(try_any_match_prefix, path, subpath, i, n, method, middlewares, has_route);
        handle_prefix(try_method_match_prefix, path, subpath, i, n, method, middlewares, has_route);
      }

      return middlewares;
    }

    void handle_prefix(PathArgument &prefix, const PathArgument &path, const std::string &subpath, size_t i, size_t n, const std::string &method, std::deque<http_middleware_func> &middlewares, bool &has_route)
    {
      std::string p{"/" + subpath}; // 第一个 subpath 是空字符，因此判断根路径
      if (i > 0)
        prefix += p; // 防止重复加 '/'

      auto &matched = middleware_map[i > 0 ? prefix : prefix + p];

      for (auto &mde : matched)
      {
        const auto &config = mde.config;
        if (http_middleware_func *func = mde.GetMiddlewareFunc(); func != nullptr)
        {
          if (!config.exact || (config.exact && i == n - 1))
          {
            middlewares.emplace_back(*func);
            has_route = config.is_route;
          }
        }
        else
        {
          http_middleware_collection *m_collection = mde.GetMiddlewareCollection();

          // 切分出子路径，递归调用
          const std::string &used_path = prefix.substr(prefix.find(MATCH_PROTO) + 1);
          const PathArgument &left_path = used_path.size() >= path.size() ? "/" : path.substr(used_path.size());

          const std::deque<http_middleware_func> &ret = m_collection->get_matched_paths(left_path, method, has_route);
          middlewares.insert(middlewares.end(), ret.begin(), ret.end());
        }
      }
    };

  public:
    http_middleware_collection() {};
    http_middleware_collection(const http_middleware_collection &other)
    {
      middleware_map = other.middleware_map;
    };
    http_middleware_collection(http_middleware_collection &&other)
    {
      middleware_map = std::move(other.middleware_map);
    }
    http_middleware_collection &operator=(const http_middleware_collection &other)
    {
      middleware_map = other.middleware_map;
      return *this;
    }
    http_middleware_collection &operator=(http_middleware_collection &&other)
    {
      middleware_map = std::move(other.middleware_map);
      return *this;
    }
    virtual ~http_middleware_collection()
    {
      middleware_map.clear();
      std::map<PathArgument, std::deque<RequestHandlerWrapper>>().swap(middleware_map);
    };
    // 注册中间件/或者中间件集合
    // 注意这些方法注册的中间件是有顺序的，先调用方法注册的中间件先执行，因此需要使用队列
    void Use(RequestHandler handler, bool exact = false)
    {
      middleware_map[compose_match_path(ROOT_PATH)].emplace_back(RequestHandlerConfig{.exact = exact}, handler);
    };

    void Use(const PathArgument &path, RequestHandler handler, bool exact = false)
    {
      middleware_map[compose_match_path(path)].emplace_back(RequestHandlerConfig{.exact = exact}, handler);
    };

    void Use(const PathArgument &path, verb method, RequestHandler handler, bool exact = false)
    {
      middleware_map[compose_match_path(path, method)].emplace_back(RequestHandlerConfig{.exact = exact}, handler);
    };
  };

  RequestHandlerWrapper::~RequestHandlerWrapper()
  {
    http_middleware_collection *ret = GetMiddlewareCollection();
    if (ret != nullptr)
      delete ret;
  };
}