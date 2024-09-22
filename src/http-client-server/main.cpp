#include "net_http/http_server.hpp"
#include "net_http/buildin-middlewares/cors.hpp"
#include "net_http/buildin-middlewares/json-body.hpp"

using namespace net::nhttp;
using namespace net::nhttp::middleware;

int main()
{
  http_server server(8080);

  server.Use(Cors());
  server.Use(JsonBody());

  server.Get("/email", [](Request &req, Response &res, NextFunction)
             { End(res, "Hello, email"); });

  server.Post("/email", [](Request &req, Response &res, NextFunction)
              {
                Json::Value root;
                auto query = req.Get<std::map<std::string, std::string>>("query");
                for (auto &kv : query)
                  root[kv.first] = kv.second;
                if (req.Has(JsonBody::GetJsonBodyName()))
                  root["req_body"] = req.Get<Json::Value>(JsonBody::GetJsonBodyName());
                root["email"] = "test@example.com";
                root["age"] = 10;
                End(res, JsonBody::json_writer.write(root));
                res.set(http::field::content_type, "application/json"); });
  server.Start();

  return 0;
}
