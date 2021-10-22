function decodeUplink(input) {
    return {
      data: Decode(input.fPort, input.bytes, 0),
      warnings: [],
      errors: []
    };
  }

  var cursor = 0;
  var buffer;

  function Decode(port, bytes, variables) {

      buffer = bytes;

      var temperature = s16() / 10.0;
      var voltage = s16() / 10.0;
      var latitude = s32() / 1E5;
      var longitude = s32() / 1E5;
      var altitude = s16();
      var satelites = u8();

      return {
          temperature: temperature,
          voltage: voltage,
          gps: {
            latitude: latitude,
            longitude: longitude,
            altitude: altitude,
            satelites: satelites
          }
      };
  }

  function s8() {
      var value = buffer.slice(cursor);
      value = value[0];
      if ((value & (1 << 7)) > 0) {
          value = (~value & 0xff) + 1;
          value = -value;
      }
      cursor = cursor + 1;
      return value;
  }

  function u8() {
      var value = buffer.slice(cursor);
      value = value[0];
      cursor = cursor + 1;
      return value;
  }

  function s16() {
      var value = buffer.slice(cursor);
      value = value[0] | value[1] << 8;
      if ((value & (1 << 15)) > 0) {
          value = (~value & 0xffff) + 1;
          value = -value;
      }
      cursor = cursor + 2;
      return value;
  }

  function u16() {
      var value = buffer.slice(cursor);
      value = value[0] | value[1] << 8;
      cursor = cursor + 2;
      return value;
  }

  function u32() {
      var value = buffer.slice(cursor);
      value = value[0] | value[1] << 8 | value[2] << 16 | value[3] << 24;
      cursor = cursor + 4;
      return value;
  }

  function s32() {
      var value = buffer.slice(cursor);
      value = value[0] | value[1] << 8 | value[2] << 16 | value[3] << 24;
      if ((value & (1 << 31)) > 0) {
          value = (~value & 0xffffffff) + 1;
          value = -value;
      }
      cursor = cursor + 4;
      return value;
  }
