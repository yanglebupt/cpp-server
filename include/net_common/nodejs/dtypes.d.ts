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

    readNumberType<T extends TNum, B extends NumberTypeConstructor<T>>(
      numType: B,
      offset: number = 0,
      littleEndian?: boolean
    ): InstanceType<B>;
  }
}
