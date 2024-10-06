#pragma once

#include "http_router.hpp"
#include "http_connection.hpp"
#include "../utils/functions.hpp"
#include "../utils/logic_systerm.hpp"
#include <algorithm>
#include <iostream>

namespace net::nhttp
{
  // server 本身也是一个路由
  class http_server : public http_router, public logic_system<Request, http_connection>
  {
  public:
    // 一些控制量
    std::vector<verb> NO_ROUTE_METHODS{verb::options};
    uint8_t keep_alive_timeout = 5;
    uint8_t max_connection_count = 5;

    http_server(uint16_t port) : m_acceptor(ctx, tcp::endpoint(tcp::v4(), port)) {}

    virtual ~http_server()
    {
      std::cout << "[SERVER] Stop!" << std::endl;
    }

    void Start()
    {
      try
      {
        asio::io_context::work idle_worker(ctx);

        // 监听退出信号
        asio::signal_set exit_signals(ctx, SIGINT, SIGTERM);
        exit_signals.async_wait([this](auto, auto)
                                { ctx.stop(); });
        // 开启消息子线程
        this->StartHandleMessages();

        // 一直循环监听请求
        WaitForClientConnection();

        std::cout << "[SERVER] Started!" << std::endl;

        // 主线程堵塞
        ctx.run();
      }
      catch (const std::exception &e)
      {
        std::cerr << "[SERVER] Exception: " << e.what() << std::endl;
        throw e;
      }
    }

    void WaitForClientConnection()
    {
      m_acceptor.async_accept([this](std::error_code ec, tcp::socket socket)
                              {
        if (!ec)
        {
          std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << std::endl;
          std::shared_ptr<http_connection>
              client = std::make_shared<http_connection>(std::move(socket), keep_alive_timeout, max_connection_count, this->InComing());
          // 回调函数结果 client 计数加一，不会释放，因为里面的回调函数，都会 shared_from_this() 来计数加一，延长生命周期
          client->ConnectToClient();
        }
        WaitForClientConnection(); });
    }

    // 通过中间件链处理请求
    void ApplyMiddleware(const std::deque<http_middleware_func> &middlewares, Request &req, Response &res, int cur, int total)
    {
      if (cur >= total)
        return;
      const http_middleware_func &md = middlewares.at(cur);
      md(req, res, [&]()
         { ApplyMiddleware(middlewares, req, res, cur + 1, total); });
    }

    // 接收到客户端消息包
    void OnMessage(std::shared_ptr<http_connection> client, Request &req) override
    {
      std::shared_ptr<Response> res = std::make_shared<Response>();
      bool need_keep_alive = client->NeedKeepAlive(req);
      res->version(req.version());

      // 解析地址，得到 query，需要为 req 添加新属性
      auto [url, search] = SplitURLSearch(req.target());
      req.Add("path", url);
      req.Add("search", search);
      req.Add("query", URLSearchParams(search));

      const std::string &method = req.method_string();
      const std::string &target = req.Get<std::string>("path");
      std::cout << client->remote_endpoint() << " " << method << " " << target << std::endl;
      bool has_route = false;
      bool need_route = std::find(NO_ROUTE_METHODS.begin(), NO_ROUTE_METHODS.end(), req.method()) == NO_ROUTE_METHODS.end();
      const std::deque<http_middleware_func> &middlewares = this->get_matched_paths(target, method, has_route);
      if (!need_route || (need_route && has_route))
        ApplyMiddleware(middlewares, req, *res, 0, middlewares.size());
      else
      {
        // 如果整个过程没有 route 中间件，则返回 404
        res->result(http::status::not_found);
        res->set(http::field::content_type, "text/plain");
        beast::ostream(res->body()) << "[" << method << " " << target << "] is not found!";
      }
      client->WriteResponse(res, need_keep_alive);
    };

  private:
    asio::io_context ctx;
    asio::ip::tcp::acceptor m_acceptor;
  };
}
