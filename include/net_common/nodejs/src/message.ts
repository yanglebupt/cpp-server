export class Message<T extends number> {
  id: T;
  size: number = 0;

  // bytes_of_id: BYTE = 4;
  // bytes_of_size: BYTE = 4;

  // body: number[] = [];

  // buffer(): Uint8Array {
  //   const array_buffers = [
  //     number2bytes(this.id, this.bytes_of_id),
  //     number2bytes(this.size, this.bytes_of_size),
  //   ];
  //   const bytes = array_buffers.reduce((pre, cur) => pre + cur.byteLength, 0);
  //   const uint8_array = new Uint8Array(bytes);
  //   let byte_offset = 0;
  //   array_buffers.forEach((buffer) => {
  //     uint8_array.set(new Uint8Array(buffer), byte_offset);
  //     byte_offset += buffer.byteLength;
  //   });
  //   return uint8_array;
  // }
}
