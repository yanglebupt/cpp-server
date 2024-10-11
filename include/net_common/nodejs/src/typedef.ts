type TNum = number | bigint;
type NumberTypeConstructor = new <T extends TNum = number>(
  value: T
) => NumberType;
export class NumberType<T extends TNum = number> {
  constructor(public value: T) {}
}

// char
export class Int8 extends NumberType {}
export class UInt8 extends NumberType {}

// short
export class Int16 extends NumberType {}
export class UInt16 extends NumberType {}

// int
export class Int32 extends NumberType {}
export class UInt32 extends NumberType {}

// long
export class BigInt64 extends NumberType<bigint> {}
export class BigUInt64 extends NumberType<bigint> {}

export class Float extends NumberType {}
export class Double extends NumberType {}

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

Buffer.prototype.readNumberType = function <T extends NumberTypeConstructor>(
  numType: T,
  offset: number = 0,
  littleEndian?: boolean
): InstanceType<T> {
  const num = new numType(0) as InstanceType<T>;
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
