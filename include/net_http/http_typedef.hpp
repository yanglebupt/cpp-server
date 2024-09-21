#pragma once

#include <boost/beast/http.hpp>
#include <functional>
#include <string>

#ifndef ROOT_PATH
#define ROOT_PATH "/"
#endif

typedef boost::beast::http::request<boost::beast::http::dynamic_body> Request;
typedef boost::beast::http::response<boost::beast::http::dynamic_body> Response;
// 后面会支持正则路径匹配
typedef std::string PathArgument;
typedef std::function<void(void)> NextFunction;
// 中间件就是一个函数
typedef std::function<void(const Request &, Response &, NextFunction next)> RequestHandler;
typedef std::function<void(const Request &, Response &)> RequestHandlerCallback;