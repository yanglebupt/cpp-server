import EventEmitter from "events";
import { NetConnectOpts, createConnection, Socket } from "net";

export class TCPClient extends EventEmitter {
  public socket: Socket;
  private buffer: number[] = [];
  constructor(options: NetConnectOpts) {
    super();
    const socket = createConnection(options);
    socket.on("connect", this.connect.bind(this));
    socket.on("error", this.error.bind(this));
    socket.on("data", this.data.bind(this));
    this.socket = socket;
    this.handle_data();
  }

  private async handle_data() {
    // ReadAccepted
    const accepted = (await this.async_read(1)).readUInt8();
    if (!accepted) return;
    console.log("Server accepted");
    // ReadValidation
    const validation = (await this.async_read(8)).readBigUInt64LE();
    const response = this.scramble(validation);
    // WriteValidation
    const buffer = Buffer.alloc(8);
    buffer.writeBigUInt64LE(response);
    await this.async_write(buffer);

    // ReadValidationResult
    const validation_ok = (await this.async_read(1)).readUInt8();
    if (!validation_ok) return;
    console.log("Server validated");

    // ReadHeader
    // ReadBody
  }

  protected message() {}

  private async async_read(bytes: number): Promise<Buffer> {
    return new Promise((resolve, reject) => {
      const id = setInterval(() => {
        if (this.buffer.length >= bytes) {
          clearInterval(id);
          resolve(Buffer.from(this.buffer.splice(0, bytes)));
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

  protected data(data: Buffer) {
    this.buffer = [...this.buffer, ...data];
  }

  protected connect() {
    console.log("Connected!");
  }

  protected error(error: Error) {
    console.log("error: ", error.name, error.message);
  }

  private scramble(nInput: bigint): bigint {
    let out = nInput ^ BigInt("0xdeadbeefc0decafe");
    out =
      ((out & BigInt("0xf0f0f0f0f0f0f0")) >> BigInt(4)) |
      ((out & BigInt("0x0f0f0f0f0f0f0f")) << BigInt(4));
    return out ^ BigInt("0xc0deface12345678");
  }
}
