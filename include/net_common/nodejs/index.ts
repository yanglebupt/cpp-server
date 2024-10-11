import { TCPClient } from "./src/client";

const client = new TCPClient({
  port: 5050,
  host: "127.0.0.1",
  keepAlive: true,
});
