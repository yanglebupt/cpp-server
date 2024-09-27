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

  http_router *rt = new http_router();

  server.Get("/", [](Request &req, Response &res, NextFunction)
             { End(res, "Hello"); });

  server.Get("/hello", [](Request &req, Response &res, NextFunction)
             { End(res, "Hello, hello"); });

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

  rt->Get("/", [](Request &req, Response &res, NextFunction)
          { End(res, "Hello, router"); });

  rt->Get("/list", [](Request &req, Response &res, NextFunction)
          { End(res, "Hello, list"); });

  rt->Post("/info", [](Request &req, Response &res, NextFunction)
           {
                Json::Value root;
                auto query = req.Get<std::map<std::string, std::string>>("query");
                for (auto &kv : query)
                  root[kv.first] = kv.second;
                if (req.Has(JsonBody::GetJsonBodyName()))
                  root["req_body"] = req.Get<Json::Value>(JsonBody::GetJsonBodyName());
                root["email"] = 182793463;
                root["gender"] = "man";
                End(res, JsonBody::json_writer.write(root));
                res.set(http::field::content_type, "application/json"); });

  server.Use("/test", rt);

  server.Start();

  return 0;
}
