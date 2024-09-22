# HTTP 通信

应用层协议，有更加复杂的、规范的消息头格式（请求/响应）。如果想要完全从 asio/tcp 开发一个 HTTP 服务器，需要自己一行一行地解析请求消息，并处理每个字段的含义，执行对应逻辑，并返回响应消息。因此我们这里采用现有的 `beast` 来开发 HTTP 服务器

## 中间件/路由/服务器

原型设计如下（参考了 expressjs 的设计理念）

- 中间件是用来处理请求的函数，多个中间件组成一条链，并通过 `next` 顺序执行。**路由函数就是一个中间件**

- 路由是负责注册路由函数（get/post），也可以注册其他中间件（use），因此路由可以看成是一个中间件集合，并且认为中间件集合和中间件是一致的
  - 中间件处理，直接调用方法执行即可
  - 中间件集合处理，需要先从集合中过滤出中间件，然后再调用方法执行
  - 会形成嵌套，中间件集合里面包括另一个中间件集合

![](./imgs/Middleware.png)

- 并且服务器是根路由，可以包含多个子路由。由于服务器本身也是一个路由，因此路由有的功能，服务器也有

![](./imgs/HttpInherit.png)

```c++
// 中间件
typedef std::function<void(const Request &, Response &, NextFunction next)> RequestHandler;

// 中间件集合
class MiddlewareCollection : public RequestHandler
{
  protected:
    // 不同路径下的中间件队列 ---> 组成中间件集合
    std::map<PathArgument, std::deque<RequestHandler>> middleware_map;
  public:
    // 注册中间件/或者中间件集合
    void Use(const PathArgument &path, RequestHandler handler);
}

// 路由
class router : public MiddlewareCollection
{
  public:
    // 注册路由函数
    void Get(const PathArgument &path, RequestHandler handler);
    void Post(const PathArgument &path, RequestHandler handler);
}

// 服务器
class http_server : public router {}
```

## Updated

实现了 JSON 中间件 (内置支持 Query) 和 跨域请求中间件

- 中间件需要对 req/res 进行增强，例如添加不同类型的属性，但是 c++ 本身不支持这种操作，难道还是只能用 JSON 吗？？
- 模板实现动态增加属性：https://www.cnblogs.com/grass-and-moon/p/13680266.html
- 中间件可以使用函数实现，但对于复杂情况尽量使用仿函数来实现，参考<a href="./include/net_http/buildin-middlewares/cors.hpp">内置的中间件</a>

实现了指定路由加中间件，并且中间件支持可以请求方法匹配了，并且可以选择是否开启精确匹配

后面考虑支持批量注册中间件

## Example

<a href="./src/http-client-server/main.cpp">基本使用</a>

# WebSocket 通信

# RPC 通信