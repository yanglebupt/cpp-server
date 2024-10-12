import { NetGlobalConfig } from "./config";
import { NumberType, NumberTypeConstructor, TNum } from "./typedef";

export type HeaderConstructorType = NumberTypeConstructor<number>;
export type HeaderType = InstanceType<HeaderConstructorType>;
export interface MessageHeader<T> {
  id: T;
  size: T;
}
export type HeaderName = keyof MessageHeader<any>;
export interface Message extends MessageHeader<HeaderType> {}
export class Message {
  // header field order
  static HeaderFields: HeaderName[] = ["id", "size"];
  constructor(private header_type: MessageHeader<HeaderConstructorType>) {}

  // body
  public body: Buffer = Buffer.alloc(0);

  // set/get headers
  public set_header(name: HeaderName, value: number) {
    Reflect.set(this, name, new this.header_type[name](value));
  }
  public get_header(name: HeaderName): number {
    return Reflect.get(this, name).value;
  }

  public get header_size(): number {
    return Object.values(this.header_type).reduce(
      (pre, cur) => pre + Reflect.get(cur, "BYTES"),
      0
    );
  }

  public set_header_from_buffer(header: Buffer) {
    let offset = 0;
    Message.HeaderFields.forEach((name) => {
      const ctr = this.header_type[name];
      Reflect.set(
        this,
        name,
        header.readNumberType(ctr, offset, NetGlobalConfig.littleEndian)
      );
      offset += Reflect.get(ctr, "BYTES");
    });
  }

  public set_body_from_buffer(body: Buffer) {
    this.body = body;
  }

  public buffer(): Buffer {
    const header = Buffer.alloc(this.header_size);
    let offset = 0;
    Message.HeaderFields.forEach((name) => {
      const field = Reflect.get(this, name);
      header.writeNumberType(field, offset, NetGlobalConfig.littleEndian);
      offset += Reflect.get(this.header_type[name], "BYTES");
    });
    return Buffer.concat([header, this.body]);
  }

  public push(data: NumberType<TNum> | Array<NumberType<TNum>> | string) {
    let buffer: Buffer;
    if (typeof data === "string") {
      buffer = Buffer.from(data);
    } else if (Array.isArray(data)) {
      const bytes = data.reduce(
        (pre, n) => pre + Reflect.get(n.constructor, "BYTES"),
        0
      );
      buffer = Buffer.alloc(bytes);
      let offset = 0;
      data.forEach((n) => {
        buffer.writeNumberType(n, offset, NetGlobalConfig.littleEndian);
        offset += Reflect.get(n.constructor, "BYTES");
      });
    } else {
      buffer = Buffer.alloc(Reflect.get(data.constructor, "BYTES"));
      buffer.writeNumberType(data, 0, NetGlobalConfig.littleEndian);
    }

    this.body = Buffer.concat([this.body, buffer]);
    this.set_header("size", this.body.length);
  }
}
