#pragma once

#include <regex>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <tuple>

/**
 * 用 delim 指定的正则表达式将字符串 in 分割，返回分割后的字符串数组
 */
std::vector<std::string> SplitString(const std::string &in, const std::string &delim)
{
  std::regex re{delim};
  // 构造函数,完成字符串分割
  return std::vector<std::string>{
      std::sregex_token_iterator(in.begin(), in.end(), re, -1),
      std::sregex_token_iterator()};
}

std::tuple<std::string, std::string> SplitURLSearch(const std::string &url)
{
  size_t pos = url.find_first_of("?");
  if (pos == std::string::npos)
    return std::make_tuple(url, "");
  else
    return std::make_tuple(url.substr(0, pos), url.substr(pos));
}

/**
 * 这里往往有更复杂的结构，数组等如何解析出来
 * 处理解析失败的异常
 */
std::map<std::string, std::string> URLSearchParams(const std::string &url)
{
  std::map<std::string, std::string> query;
  size_t pos = url.find_first_of("?");
  if (pos == std::string::npos || pos == url.size() - 1)
    return query;
  auto pairs = SplitString(url.substr(pos + 1), "&");
  for (auto pair : pairs)
  {
    auto kv = SplitString(pair, "=");
    if (kv.size() == 0)
      continue;
    else if (kv.size() == 1)
      query[kv[0]] = "";
    else if (kv.size() == 2)
      query[kv[0]] = kv[1];
  }
  return query;
};