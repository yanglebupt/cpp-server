#include <iostream>
#include "asio.hpp"
#include <thread>

// asio::io_context 和异步操作：https://www.jianshu.com/p/d6ae8adb5914

std::vector<char> vbuf(20 * 1024);

void GrabSomeData(asio::ip::tcp::socket &socket)
{
  socket.async_read_some(asio::buffer(vbuf.data(), vbuf.size()), [&socket](std::error_code ec, size_t length)
                         {
    if(!ec){
      std::cout << "Read: " << length << std::endl;
      for (size_t i = 0; i < length; i++)
      {
        std::cout << vbuf[i];
      }
      
      GrabSomeData(socket);
    } });
}

int main()
{

  asio::error_code ec;

  asio::io_context ctx;
  asio::io_context::work idleWork(ctx);
  std::thread ctxThread = std::thread([&ctx]()
                                      { ctx.run(); });

  asio::ip::tcp::endpoint endpoint(asio::ip::make_address("51.38.81.49"), 80);

  asio::ip::tcp::socket socket(ctx);

  socket.connect(endpoint, ec);

  if (!ec)
  {
    std::cout << "Connected!" << std::endl;
  }
  else
  {
    std::cout << "No Connec" << std::endl;
  }

  if (socket.is_open())
  {
    std::string req = "GET /index.html HTTP/1.1\r\n"
                      "Host: david-barr.co.uk\r\n"
                      "Connection: close\r\n\r\n";
    socket.write_some(asio::buffer(req.data(), req.size()), ec);

    GrabSomeData(socket);

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    ctx.stop();
    if (ctxThread.joinable())
      ctxThread.join();
  }

  std::cout << "Over" << std::endl;
  return 0;
}