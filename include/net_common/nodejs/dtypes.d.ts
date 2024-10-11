namespace globalThis {
  interface Buffer {
    findFunc<T extends TNum>(
      num: NumberType<T>,
      read_func: boolean,
      littleEndian?: boolean
    ): Function;

    writeNumberType<T extends TNum>(
      num: NumberType<T>,
      offset: number = 0,
      littleEndian?: boolean
    ): void;

    readNumberType<T extends NumberTypeConstructor>(
      numType: T,
      offset: number = 0,
      littleEndian?: boolean
    ): InstanceType<T>;
  }
}
