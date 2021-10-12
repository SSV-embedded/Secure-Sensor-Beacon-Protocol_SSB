# Secure Sensor Beacon Protocol SSB
## Introduction
The SSV Secure Sensor Beacon Protocol (SSV/SSB) is a **Bluetooth-based sensor radio technology** for machine, plant and building automation.

SSV/SSB enables the user to equip a technical facility with additional radio sensors to obtain a more accurate real-time data image. This data image can be used, for example, to derive an extended reference variable for a control system to improve the overall efficiency of a plant.

Since each SSB node (like the Smart Factory Sensor SFS/BE1 from SSV) is factory-equipped with an **individual 128-bit key**, the sensor data receiver can securely verify both source **authenticity and data integrity**. Therefore, SSV/SSB measurements are also suitable for automatic decision making in control and regulation applications.

SSV/SSB is based on the widely used Bluetooth Low Energy (BLE) beacon idea: a BLE device periodically sends short messages as broadcast telegrams. Any BLE-enabled system with appropriate software that is within range can receive and evaluate these messages, e.g., a smartphone with a suitable app. In this way, sensor measurements in a beacon can also be sent as user data via broadcast.

In accordance with the Bluetooth protocol specifications, BLE devices can send **advertising packets** (Advertising Channel PDUs, PDU = Protocol Data Units) as broadcasts. The data area (Advertising Data or ADV Data) of such an Advertising Channel PDU is used by various manufacturer-specific beacon protocols for different tasks.

Three channels are reserved for sending advertising packets (the **advertising channels CH37, CH38 and CH39** in the 2.4 GHz frequency band). Two widely used examples of BLE beacon protocols are Apple's iBeacon and Google's Eddystone. The disadvantages of existing BLE beacon protocols are primarily the lack of security against cyber attacks and the small amount of payload data that can be transmitted using an Advertising Channel PDU.

## Advantages
The Secure Sensor Beacon Protocol developed by SSV has the following advantages over other beacon protocols:

* Support for a wide variety of sensor data formats through fragmentation, i.e., measurement data is split among different Advertising Channel PDUs.
* Verifiable data integrity and sender authentication via Message Authentication Code (MAC)
* Optional AES-128 encryption for confidential sensor data transmission
* Factory pre-programmed individual 128-bit key
* Optional security pairing between sender and receiver using SSV's SFS Config App
* Very extensive power management for battery operation possible through beacon operation
* No fixed pairing to a specific gateway

## Overview
The following figure shows the structure of an SSV/SSB beacon.

![Schema of the SSV/SSB protocol](https://user-images.githubusercontent.com/85748650/136937300-ee345fa8-f3b3-4ece-aeca-1995e0b0a225.png)

## Details
### Manufacturer ID
This field is used for the manufacturer ID as assigned by the Bluetooth SIG. For instance, the SFS/BE1 uses `0x0059` (Nordic Semiconductor ASA).

### Packet Type
The value of this field specifies which hash function is used to create the HMAC.

`0x40`: BLAKE2s

`0x41`: AES-CCM

All other values are reserved for future use.

### Sequence Number
This 32-bit field contains a 27-bit counter for each sequence of received sensor data. This counter is strictly monotonically increasing and is used to protect against replay attacks. The remaining 5 bits are reserved for the fragment number. 
 
### Sequence Number + Fragment Number
This 32-bit field is a combination of the 27-bit sequence number and a 4-bit fragment counter. It additionally includes the **last fragment bit** as a flag to detect the last fragment of a sequence. The value of this field is generated as follows:

1. Take the value of the sequence counter
2. Shift it 5 times left as a logical operation
3. Add the last fragment flag on the 5th-bit
4. Use the lower nibble of the LSB as a 4-bit fragment counter

The following C code shows this operation:

```c
struct seq_frag {
    uint32_t seq;
    uint32_t frag;
    bool last_frag;
};

void pack_seq_frag (const struct seq_frag *in, uint32_t *out) {
    uint32_t seq = in->seq << 5;
    uint32_t last_frag_flag = in->last_frag ? 0x10 : 0x00;
    uint32_t frag = in->frag & 0x0f;
    *out = seq | last_frag_flag | frag;
}

void unpack_seq_frag (const uint32_t *in, struct seq_frag *out) {
    out->seq = *in >> 5;
    out->frag = *in & 0x0f;
    out->last_frag = *in & 0x10;
}
```

### HMAC
The HMAC is a 48-bit hash that is created from the sensor data and the secret using a cryptographic function. The packet type field defines which hash function is used.

### Secret
The secret is an individual 128-bit key that must be programmed into an SSB-capable device at the factory.

### Value type
This 8-bit value is used to identify the type of sensor or physical unit. The identifier has a certain structure for easier handling of combined values (e.g. x-,y-,z-coordinates).

| Glob | Typ | Num |
| ---- | --- | --- |
| 1    | 5   | 2   |

`Glob`: Defines, if it is a global list or not

`Typ`: Defines the sensor type (e.g. magnetometer, temperature...)

`Num`: Number of the parameter (e.g. a sensor with x-,y-,z-coordinates for one parameter)

#### Global Sensor Types
These definitions are examples for the SFS/BE1 of SSV.

| Parameter          | Sensor    | Bits       | Hex  |
| --------------:    | --------- | ---------- | ---- |
| Magnetic field (x) | BMM150    | 1 00000 00 | 0x80 |
| Magnetic field (y) | BMM150    | 1 00000 01 | 0x81 |
| Magnetic field (z) | BMM150    | 1 00000 10 | 0x82 |
| Temperature        | BME680    | 1 00001 00 | 0x84 |
| Pressure           | BME680    | 1 00010 00 | 0x88 |
| Humidity           | BME680    | 1 00011 00 | 0x8C |
| Gas resistance     | BME680    | 1 00100 00 | 0x90 |
| CO2                | CCS811    | 1 00110 00 | 0x98 |
| VOC                | CCS811    | 1 00111 00 | 0x9C |
| Amb. light (lux)   | OPT3001   | 1 01000 00 | 0xA0 |
| Power fail         | ---       | 1 11111 11 | 0xFF |

## Example Beacon
While the Bluetooth SIG has defined standard ways to represent sensor data via Generic Attribute Profile (GATT) connection, there is no such standard for broadcasting the sensor information. Advantages of broadcasting sensor data are simplicity, as there is no connection negotiation required and density, as a single receiver can listen to hundreds of sensors.

An SSB device like SSV's SFS/BE1 broadcasts its sensor data by default in an advertising packet as manufacturer specific data. Please refer to the Bluetooth SIG for a **[detailed explanation on Bluetooth advertising packets](https://www.bluetooth.com/blog/bluetooth-low-energy-it-starts-with-advertising/)**.

**Example of a complete advertising packet of an SFS/BE1 with BME680 sensor:**

`0x0201041BFF590040206B080004841F85A941048812C3C942048CBCF4614204`

The advertising packet is devided into following chunks:

`0x02 01 04` `1B` `FF` `5900` `40` `206B0800` `04841F85A941 048812C3C942 048CBCF46142 04`

The particular chunks are interpreted as follows:

### BLE Advertising PDU Flags
`0x02 01 04`
* `0x02`: **Length** -> 3 bytes = 1 byte length + 2 bytes payload
* `0x01`: **Data Type** -> Here: Flags
* `0x04`: **Advertising Mode** -> Here: BR/EDR not supported

---

### SSV/SSB Specific Payload
`0x1B` `FF` `5900` `40` `206B0800` `04841F85A941 048812C3C942 048CBCF46142 04`
* `0x1B`: **Payload Length** -> 28 bytes = 1 byte length + 27 bytes payload 
* `0xFF`: **ADV Type** -> Here: Manufacturer specific data
* `0x5900`: **Manufacturer ID** (LSB first) -> Here: Nordic Semiconductor ASA
* `0x40`: **Packet Type** -> Here: BLAKE2s
* `0x206B0800`: **Sequence Number + Fragment Number** (LSB first) -> `0x00086B20` -> `0b10000110101100100000`<br/>
   * Take the *bits 0..4 of the LSB* to get the information about the fragment counter -> `0b00000`<br/>
   The *bits 0..3* are the current fragment number, *bit 4* is the *last fragment flag* (1 = last fragment) -> `0b0 0000` = fragment number 0, not the last fragment (flag = 0)
   * Take the *remaining most significant bits* (e.g. bits 5..19) to get the current sequence number -> `0b100001101011001` =  17241

* `0x04841F85A941 048812C3C942 048CBCF46142 04`: **Payload Fragment 0** (LSB first) -> `0x04 84 1F85A941` , `0x04 88 12C3C942` , `0x04 8C BCF46142` , `0x04...`<br/>
    * `0x04 84 1F85A941`: value length 4, value type 0x84 = "temp", value (LSB first) = `0x41A9851F` -> `0b01000001101010011000010100011111`<br/>
    IEEE 754 floating-point number -> `0b0 0b10000011 0b01010011000010100011111` (sign, exponent, fraction)
        * sign = 0
        * exponent = 131-127 = 4
        * fraction = 1 **,** 01010011000010100011111 -> shift 4 -> 10101 **,** 0011000010100011111 -> 21 + 0,125 + 0,0625 ... = ~21,1875
        
    > **More information is provided in this [Wikipedia article about IEEE 754 floating-point numbers.](https://en.wikipedia.org/wiki/IEEE_754)**

    * `0x04 88 12C3C942`: value length 4, value type 0x88 = "pressure", value (LSB first) = `0x42C9C312` -> `0b01000010110010011100001100010010`<br/>
    IEEE 754 floating-point number -> `0b0 0b10000101 0b10010011100001100010010` (sign, exponent, fraction)
        * sign = 0
        * exponent = 133-127 = 6
        * fraction = 1 **,** 10010011100001100010010 -> shift 6 -> 1100100 **,** 11100001100010010 -> 100 + 0,5 + 0,25 + 0,125 ... = ~100,875
        
    * `0x04 8C BCF46142`: value length 4, value type 0x8C = "hum", value (LSB first) = `0x4261F4BC` -> `0b01000010011000011111010010111100`<br/>
    IEEE 754 floating-point number -> `0b0 0b10000100 0b11000011111010010111100` (sign, exponent, fraction)
        * sign = 0
        * exponent = 132-127 = 5
        * fraction = 1 **,** 11000011111010010111100 -> shift 5 -> 111000 **,** 011111010010111100 -> 56 + 0,25 + 0,125 + 0,0625 ... = ~56,4375
        
    * `0x04`: This is the first byte of the next sensor value. The remaining part of this sensor value is sent in the next payload fragment (number 1) of the current sequence (number 17241) because the maximum length of 19 bytes for the current payload fragment has been reached. The current sequence ends when the *last fragment flag* in one of the following payload fragments is 1.  

### Scan Response
Example of a typical scan response:

`0x0F 09 5346532F4245312D424D45363830`

* `0x0F`: Complete length of scan response -> 16 bytes = 1 byte length + 15 bytes payload
* `0x09`: Type of message -> Here: scan response, complete local name
* `0x5346532F4245312D424D45363830`: ASCII string (LSB first) -> Here: "SFS/BE1-BME680"
