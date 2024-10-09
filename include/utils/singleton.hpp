#pragma once

#include <memory>
#include <mutex>

template <typename Class>
class enable_private_make_shared
{
protected:
  template <typename... Args>
  static std::shared_ptr<Class> create(Args &&...args)
  {
    struct make_shared_helper : public Class
    {
      make_shared_helper(Args &&...a) : Class(std::forward<Args>(a)...) {}
    };
    return std::make_shared<make_shared_helper>(std::forward<Args>(args)...);
  }
};

template <typename T>
class singleton : public enable_private_make_shared<T>
{
protected:
  // 防止外部构造和解析
  singleton() = default;
  singleton(const singleton<T> &) = delete;
  singleton &operator=(const singleton<T> &) = delete;

private:
  // 智能指针作为单例
  inline static std::shared_ptr<T> _instance = nullptr;
  inline static std::mutex _mtx = std::mutex();

public:
  ~singleton() {};
  static std::shared_ptr<T> Instance()
  {
    std::unique_lock<std::mutex> lock(_mtx);
    if (_instance == nullptr)
      _instance = enable_private_make_shared<T>::create();
    return _instance;
  }

  static void Reset()
  {
    std::unique_lock<std::mutex> lock(_mtx);
    _instance.reset();
  }
};