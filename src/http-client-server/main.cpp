#include "net_http/http_server.hpp"

int main()
{
  net::nhttp::http_server server(8080);
  server.Get("/", [](const Request &req, Response &res, NextFunction)
             {
      res.set(http::field::content_type, "text/plain");
      beast::ostream(res.body()) << "Hello Middleware"; });
  server.Start();
  return 0;
}
