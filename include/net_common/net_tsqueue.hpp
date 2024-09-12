#pragma once

#include <mutex>
#include <deque>
#include <stddef.h>
#include <condition_variable>

namespace net
{
  /**
   * thread safe queue
   */
  template <typename T>
  class tsqueue
  {
  protected:
    std::mutex _mutex;
    std::deque<T> dq;

    // 取的条件变量
    std::condition_variable cond;

  public:
    tsqueue() = default;
    tsqueue(const tsqueue &) = delete;

    void free()
    {
      clear();
      std::deque<T>().swap(dq);
      cond.notify_all();
    }

    ~tsqueue()
    {
      free();
    };

    // 这里能返回引用吗？
    T &front()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      return dq.front();
    }

    T &back()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      return dq.back();
    }

    // 如果这里是单纯的函数模板，可以使用万能引用和完美转发
    // 但是这里是类里面的函数，并不符合万能引用的条件，因此需要手动创建两组参数
    // 传入右值，移动
    void emplace_back(T &&item)
    {
      std::lock_guard<std::mutex> lock(_mutex);
      dq.emplace_back(std::move(item));
      cond.notify_all();
    }

    void emplace_front(T &&item)
    {
      std::lock_guard<std::mutex> lock(_mutex);
      dq.emplace_front(std::move(item));
      cond.notify_all();
    }

    // 传入左值，拷贝
    void emplace_back(const T &item)
    {
      std::lock_guard<std::mutex> lock(_mutex);
      dq.emplace_back(item);
      cond.notify_all();
    }

    void emplace_front(const T &item)
    {
      std::lock_guard<std::mutex> lock(_mutex);
      dq.emplace_front(item);
      cond.notify_all();
    }

    bool empty()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      return dq.empty();
    }

    void wait()
    {
      std::unique_lock<std::mutex> lock(_mutex);
      while (dq.empty())
      {
        cond.wait(lock);
      }
    }

    size_t count()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      return dq.size();
    }

    void clear()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      dq.clear();
    }

    void remove_front()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      dq.pop_front();
    }

    void remove_back()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      dq.pop_back();
    }

    // 这里不能返回引用，pop 之后就没了，front 地址成了野指针，千万不能用，这属于未定义行为，不合法的
    T pop_front()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      // 那取元素如何防止拷贝呢？这里不能引用
      T v = dq.front();
      dq.pop_front();
      return v;
    }

    T pop_back()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      T v = dq.back();
      dq.pop_back();
      return v;
    }
  };
}