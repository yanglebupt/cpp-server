#pragma once

#include <memory>
#include <mutex>

namespace net
{
  template <typename T>
  class singleton
  {
  protected:
    // 防止外部构造和解析
    singleton() = default;
    singleton(const singleton<T> &) = delete;
    singleton &operator=(const singleton<T> &) = delete;

  private:
    // 智能指针作为单例
    inline static std::shared_ptr<T> _instance = nullptr;
    static std::once_flag _flag;

  public:
    ~singleton() {};
    static std::shared_ptr<T> Instance()
    {
      std::call_once(_flag, [&]()
                     { _instance = std::make_shared<T>(new T); });
      return _instance;
    }
  };
}