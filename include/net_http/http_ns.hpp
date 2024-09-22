#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>

namespace net::nhttp
{
  namespace beast = boost::beast;
  namespace http = boost::beast::http;
  namespace asio = boost::asio;
  using tcp = boost::asio::ip::tcp;
  using verb = boost::beast::http::verb;
}
