export type TNum = number | bigint;
export type NumberTypeConstructor<T extends TNum = number> = new (
  value: T
) => NumberType;
export abstract class NumberType<T extends TNum = number> {
  constructor(public value: T) {}
}

// char
export class Int8 extends NumberType {
  static readonly BYTES = Int8Array.BYTES_PER_ELEMENT;
}
export class UInt8 extends NumberType {
  static readonly BYTES = Uint8Array.BYTES_PER_ELEMENT;
}

// short
export class Int16 extends NumberType {
  static readonly BYTES = Int16Array.BYTES_PER_ELEMENT;
}
export class UInt16 extends NumberType {
  static readonly BYTES = Uint16Array.BYTES_PER_ELEMENT;
}

// int
export class Int32 extends NumberType {
  static readonly BYTES = Int32Array.BYTES_PER_ELEMENT;
}
export class UInt32 extends NumberType {
  static readonly BYTES = Uint32Array.BYTES_PER_ELEMENT;
}

// long
export class BigInt64 extends NumberType<bigint> {
  static readonly BYTES = BigInt64Array.BYTES_PER_ELEMENT;
}
export class BigUInt64 extends NumberType<bigint> {
  static readonly BYTES = BigUint64Array.BYTES_PER_ELEMENT;
}

export class Float extends NumberType {
  static readonly BYTES = Float32Array.BYTES_PER_ELEMENT;
}
export class Double extends NumberType {
  static readonly BYTES = Float64Array.BYTES_PER_ELEMENT;
}

Buffer.prototype.findFunc = function <T extends TNum>(
  num: NumberType<T>,
  read_func: boolean,
  littleEndian?: boolean
): Function {
  const endian = littleEndian ? "LE" : "BE";
  const type = num.constructor.name;
  const single = num instanceof UInt8 || num instanceof Int8;
  const func: Function = Reflect.get(
    this,
    (read_func ? "read" : "write") + type + (single ? "" : endian)
  );
  return func.bind(this);
};

Buffer.prototype.readNumberType = function <T extends TNum>(
  numType: NumberTypeConstructor<T>,
  offset: number = 0,
  littleEndian?: boolean
): InstanceType<NumberTypeConstructor<T>> {
  const num = new numType(0 as any) as InstanceType<NumberTypeConstructor<T>>;
  num.value = this.findFunc(num, true, littleEndian)(offset);
  return num;
};

Buffer.prototype.writeNumberType = function <T extends TNum>(
  num: NumberType<T>,
  offset: number = 0,
  littleEndian?: boolean
): void {
  this.findFunc(num, false, littleEndian)(num.value, offset);
};
