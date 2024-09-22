#pragma once

#include "../http_typedef.hpp"

namespace net::nhttp::middleware
{
  enum class origin_type
  {
    ANY,
    AUTO,
    CUSTOM,
  };

  struct CorsConfig
  {
    origin_type o_type = origin_type::AUTO;
    beast::string_view origins;
    beast::string_view headers = "Content-Type";
    beast::string_view methods = "PUT,POST,GET,DELETE,OPTIONS";
    bool credentials = true;
  };

  struct Cors : public RequestHandler
  {
  private:
    CorsConfig config;

  public:
    Cors() : config({}) {}
    Cors(const CorsConfig &config) : config(config) {}
    void operator()(Request &req, Response &res, NextFunction next)
    {
      if (config.o_type == origin_type::ANY)
        res.set(http::field::access_control_allow_origin, "*");
      else if (config.o_type == origin_type::AUTO)
        res.set(http::field::access_control_allow_origin, req.find(http::field::origin) != req.end() ? req.at(http::field::origin) : config.origins);
      else
        res.set(http::field::access_control_allow_origin, config.origins);
      res.set(http::field::access_control_allow_headers, config.headers);
      res.set(http::field::access_control_allow_methods, config.methods);
      res.set(http::field::access_control_allow_credentials, config.credentials ? "true" : "false");
      next();
    }
  };

}