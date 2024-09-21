#include <iostream>
#include "net_http/timer.hpp"

int main()
{
  asio::io_context ctx;

  // 注意一定要接收返回值，不然将亡值直接析构了
  auto timer = net::nhttp::timeinterval(ctx, []()
                                        { std::cout << "Time out" << std::endl; }, 1000);

  ctx.run();

  return 0;
}