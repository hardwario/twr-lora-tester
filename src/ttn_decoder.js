function Decoder(bytes, port) {
    // Decode an uplink message from a buffer
    var temperature = ((bytes[0]) | bytes[1] << 8) / 10.0;
    var latitude = (bytes[2] | bytes[3] << 8 | bytes[4] << 16 | bytes[5]<<24) / 1E5
    var longitude = (bytes[6] | bytes[7] << 8 | bytes[8] << 16 | bytes[9]<<24) / 1E5
    var altitude = bytes[10] | bytes[11] << 8;
    var satellites = bytes[12];

    // (array) of bytes to an object of fields.
    var decoded = {
      temperature: temperature,
      latitude: latitude,
      longitude: longitude,
      altitude: altitude,
      sats: satellites

    };

    // if (port === 1) decoded.led = bytes[0];

    return decoded;
  }


  function Validator(converted, port) {
    // Return false if the decoded, converted
    // message is invalid and should be dropped.

    if (converted.latitude === 0 || converted.longitude === 0) {
       return false;
    }

    return true;
  }
