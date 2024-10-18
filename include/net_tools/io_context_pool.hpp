#pragma once

#include "../utils/singleton.hpp"
#include <thread>
#include <vector>
#include <memory>
#include "asio.hpp"

class io_context_pool : public singleton<io_context_pool>
{
  friend class singleton<io_context_pool>;
  friend class enable_private_make_shared<io_context_pool>;

public:
  ~io_context_pool() {}

  unsigned int size;

  asio::io_context &GetIOContext()
  {
    cur_idx = (cur_idx + 1) % size;
    return ctxs[cur_idx];
  };

  void Stop()
  {
    for (unsigned int i = 0; i < size; i++)
    {
      // work 释放后，如果没有异步任务了，io_context 也会释放
      works[i].reset();
      // 此时如果有发送任务，那么会等到发送队列为空，全部发送完毕，发送回调则不再注册任务了
      // 但是有读任务的时候，读回调中一直在注册任务
      if (threads[i].joinable())
        threads[i].join();
    }
    io_context_pool::Reset();
  };

  void Start()
  {
    threads.reserve(size);
    for (unsigned int i = 0; i < size; i++)
    {
      // 由于是在子线程里面 run，必须确保始终存在一个 work
      works[i] = std::make_unique<asio::io_context::work>(ctxs[i]);
      threads.emplace_back([this, i]()
                           { ctxs[i].run(); });
    }
  }

private:
  io_context_pool() : size(std::thread::hardware_concurrency() - 2) {};
  io_context_pool(unsigned int size) : size(size) {};

  int cur_idx = -1;
  std::vector<std::thread> threads;
  std::vector<asio::io_context> ctxs{size};
  std::vector<std::unique_ptr<asio::io_context::work>> works{size};
};