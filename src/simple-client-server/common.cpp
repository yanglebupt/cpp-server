#pragma once
#include <type_traits>
#include <cstdint>
#include <iostream>
#include <conio.h>
#include <functional>

template <typename T>
typename std::underlying_type<T>::type PrintEnum(T const value)
{
  return static_cast<typename std::underlying_type<T>::type>(value);
}

class CMDInputListener
{
private:
  std::string line;

public:
  // 通过回调函数监听并处理每一行输入，回调不允许是异步函数
  void operator()(std::function<void(const std::string &)> _Line_Callback, bool hidden = false, std::function<void(char, const std::string &)> _Char_Callback = nullptr)
  {
    char ch;
    if (_kbhit())
    {                // 检查是否有输入
      ch = _getch(); // 获取输入字符
      if (ch == 13)
      {
        std::cout << std::endl;
        _Line_Callback(line);
        line.clear();
      }
      else
      {
        if (!hidden)
          std::cout << ch;
        if (_Char_Callback != nullptr)
          _Char_Callback(ch, line);
        line += ch;
      }
    }
  }
};

enum class CustomMsgType : uint32_t
{
  ServerPing,
  MessageAll,
  ServerMessage,
};

struct custom_header : net::header
{
  SERIALIZE(size, id);
  CustomMsgType id;
};