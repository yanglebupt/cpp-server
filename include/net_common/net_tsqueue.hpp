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
    ~tsqueue()
    {
      dq.clear();
      std::deque<T>().swap(dq);
    };

    const T &front()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      return dq.front();
    }

    const T &back()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      return dq.back();
    }

    void emplace_back(const T &item)
    {
      std::lock_guard<std::mutex> lock(_mutex);
      dq.emplace_back(std::move(item));
      cond.notify_all();
    }

    void emplace_front(const T &item)
    {
      std::lock_guard<std::mutex> lock(_mutex);
      dq.emplace_front(std::move(item));
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
      auto t = std::move(dq.front());
      dq.pop_front();
      return t;
    }

    T pop_back()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      auto t = std::move(dq.back());
      dq.pop_back();
      return t;
    }
  };
}