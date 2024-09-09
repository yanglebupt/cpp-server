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

    // 返回不能用 std::move，后面右值是无法取地址的
    // 但是由于添加的时候是 std::move 添加右值的
    T front()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      return dq.front();
    }

    T back()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      return dq.back();
    }

    // 注意参数不要 const T item，const 无法移动，仍然调用拷贝构造函数
    void emplace_back(T &item)
    {
      std::lock_guard<std::mutex> lock(_mutex);
      dq.emplace_back(std::move(item));
      cond.notify_all();
    }

    void emplace_back(T &&item)
    {
      std::lock_guard<std::mutex> lock(_mutex);
      dq.emplace_back(item);
      cond.notify_all();
    }

    void emplace_front(T &item)
    {
      std::lock_guard<std::mutex> lock(_mutex);
      dq.emplace_front(std::move(item));
      cond.notify_all();
    }

    void emplace_front(T &&item)
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

    T pop_front()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      auto v = dq.front();
      dq.pop_front();
      return v;
    }

    T pop_back()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      auto v = dq.back();
      dq.pop_back();
      return v;
    }
  };
}