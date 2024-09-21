#pragma once

#include "http_ns.hpp"
#include "http_router.hpp"
#include "http_connection.hpp"
#include <iostream>

namespace net::nhttp
{
  // server 本身也是一个路由
  class http_server : public router
  {
  public:
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
              client = std::make_shared<http_connection>(std::move(socket));
          // 回调函数结果 client 计数减一，不会释放，因为里面的回调函数，都会 shared_from_this() 来计数加一，延长生命周期
          client->ReadRequest([this](const Request &req, Response &res)
                              { 
                                std::string &&method = req.method_string();
                                std::string &&target = req.target();
                                std::cout << method << " " << target << std::endl;
                                // 目前只过滤请求 target，后续会添加过滤请求 method 的功能
                                ApplyMiddleware(middleware_map[target], req, res); });
        }
        WaitForClientConnection(); });
    }

    // 通过中间件链处理请求
    void ApplyMiddleware(std::deque<RequestHandler> &middlewares, const Request &req, Response &res)
    {
      if (middlewares.empty())
        return;
      RequestHandler &md = middlewares.front();
      md(req, res, [&]() mutable
         { middlewares.pop_front();  // 不能弹出来，应该设置一个索引
          ApplyMiddleware(middlewares, req, res); });
    }

  private:
    asio::io_context ctx;
    asio::ip::tcp::acceptor m_acceptor;
  };
}
