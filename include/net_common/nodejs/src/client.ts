import EventEmitter from "events";
import { NetConnectOpts, createConnection, Socket } from "net";
import { Message } from "./message";
import { BigUInt64, UInt32 } from "./typedef";
import { NetGlobalConfig } from "./config";

// 指明回调参数列表类型
export interface TCPClientEvent {
  connected: [void];
  message: [Message];
  error: [Error];
}
export class TCPClient extends EventEmitter<TCPClientEvent> {
  public socket: Socket;
  private incoming: number[] = [];
  private sendout: Message[] = [];
  constructor(options: NetConnectOpts) {
    super();
    const socket = createConnection(options);
    socket.on("connect", this.connect.bind(this));
    socket.on("error", this.error.bind(this));
    socket.on("data", this.data.bind(this));
    this.socket = socket;
    this.handle_incoming();
  }

  private async handle_incoming() {
    // ReadAccepted
    const accepted = (await this.async_read(1)).readUInt8();
    if (!accepted) return;
    console.log("Server accepted");
    // ReadValidation
    const validation = (await this.async_read(8)).readNumberType(
      BigUInt64,
      0,
      NetGlobalConfig.littleEndian
    );
    const response = this.scramble(validation);
    // WriteValidation
    const buffer = Buffer.alloc(8);
    buffer.writeNumberType(response, 0, NetGlobalConfig.littleEndian);
    await this.async_write(buffer);

    // ReadValidationResult
    const validation_ok = (await this.async_read(1)).readUInt8();
    if (!validation_ok) return;
    console.log("Server validated");

    // 可以开始接收了
    this.incoming_message();
    // 可以开始发送了
    this.sendout_message();
  }

  private async fetch_message() {
    return new Promise<Message>((resolve, reject) => {
      const id = setInterval(() => {
        if (this.sendout.length > 0) {
          clearInterval(id);
          resolve(this.sendout.shift()!);
        }
      }, 100);
    });
  }

  // WriteHeader——WriteBody
  private async sendout_message() {
    const msg = await this.fetch_message();
    await this.async_write(msg.buffer());
    this.sendout_message();
  }

  // ReadHeader——ReadBody
  private async incoming_message() {
    const message = new Message({ id: UInt32, size: UInt32 });
    message.set_header_from_buffer(await this.async_read(message.header_size));
    message.set_body_from_buffer(
      await this.async_read(message.get_header("size"))
    );
    this.message(message);
    this.incoming_message();
  }

  /////////////// emit events //////////////////////
  protected async message(message: Message) {
    this.emit("message", message);
  }
  protected connect() {
    this.emit("connected");
  }
  protected error(error: Error) {
    this.emit("error", error);
  }
  /////////////////////////////////////

  public send(message: Message) {
    this.sendout.push(message);
  }

  private async async_read(bytes: number): Promise<Buffer> {
    return new Promise((resolve, reject) => {
      const id = setInterval(() => {
        if (this.incoming.length >= bytes) {
          clearInterval(id);
          resolve(Buffer.from(this.incoming.splice(0, bytes)));
        }
      }, 100);
    });
  }

  private async async_write(buffer: Buffer): Promise<number> {
    return new Promise((resolve, reject) => {
      this.socket.write(buffer, (err?: Error) => {
        if (err) reject(err);
        else resolve(buffer.byteLength);
      });
    });
  }

  private data(data: Buffer) {
    this.incoming = [...this.incoming, ...data];
  }

  private scramble(nInput: bigint | BigUInt64): BigUInt64 {
    if (typeof nInput !== "bigint") nInput = nInput.value;
    let out = nInput ^ BigInt("0xdeadbeefc0decafe");
    out =
      ((out & BigInt("0xf0f0f0f0f0f0f0")) >> BigInt(4)) |
      ((out & BigInt("0x0f0f0f0f0f0f0f")) << BigInt(4));
    return new BigUInt64(out ^ BigInt("0xc0deface12345678"));
  }
}
