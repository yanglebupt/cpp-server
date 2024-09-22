#pragma once

#include "../http_typedef.hpp"
#include "json/json.h"

namespace net::nhttp::middleware
{
  struct JsonBody : public RequestHandler
  {
    inline static Json::Reader json_reader = Json::Reader();
    inline static Json::FastWriter json_writer = Json::FastWriter();
    inline static std::string GetJsonBodyName() { return "json_body"; };

    JsonBody() {};

    void operator()(Request &req, Response &res, NextFunction next)
    {
      if (req.find(http::field::content_type) != req.end() && (req.at(http::field::content_type) == "application/json" || req.at(http::field::content_type) == "text/json"))
      {
        std::string b_str = beast::buffers_to_string(req.body().data());
        Json::Value body;
        if (json_reader.parse(b_str, body))
          req.Add(GetJsonBodyName(), std::move(body));
      }
      next();
    };
  };

}