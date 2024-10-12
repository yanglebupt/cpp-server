import { TCPClient } from "./src/client";
import { NetGlobalConfig } from "./src/config";
import { Message } from "./src/message";
import { BigUInt64, UInt32 } from "./src/typedef";

// 规定 id 枚举的 bytes，并且枚举只能是整形，顺序要和服务端一致
enum CustomMsgType {
  ServerPing,
  MessageAll,
  ServerMessage,
}

const client = new TCPClient({
  port: 5050,
  host: "127.0.0.1",
  keepAlive: true,
});

client.on("connected", () => {
  console.log("Connected!");
});

client.on("error", (error) => {
  console.log("error: ", error.name, error.message);
});

client.on("message", (message: Message) => {
  const id = message.get_header("id");
  if (id == CustomMsgType.ServerPing) {
    const time =
      BigInt(Date.now()) -
      message.body.readNumberType(BigUInt64, 0, NetGlobalConfig.littleEndian)
        .value;
    console.log(`Server ping: ${Number(time) * 1e-6}`);
  } else if (id == CustomMsgType.ServerMessage) {
    const sp = message.body.length - 4;
    const json_str = message.body.toString("utf-8", 0, sp);
    const client_id = message.body.readNumberType(
      UInt32,
      sp,
      NetGlobalConfig.littleEndian
    ).value;
    console.log(`Hello from [${client_id}], ${json_str}`);
  }
});

// ServerPing
{
  const msg = new Message({ id: UInt32, size: UInt32 });
  msg.set_header("id", CustomMsgType.ServerPing);
  msg.push(new BigUInt64(BigInt(Date.now())));
  client.send(msg);
}

// MessageAll
{
  const msg = new Message({ id: UInt32, size: UInt32 });
  msg.set_header("id", CustomMsgType.MessageAll);
  msg.push(JSON.stringify({ name: "Mike", age: 20 }));
  client.send(msg);
}
