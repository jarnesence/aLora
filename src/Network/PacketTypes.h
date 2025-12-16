#ifndef PACKET_TYPES_H
#define PACKET_TYPES_H

#include <Arduino.h>

enum PacketType : uint8_t {
    PACKET_TYPE_BEACON = 0x01,
    PACKET_TYPE_HANDSHAKE_REQ = 0x02,
    PACKET_TYPE_HANDSHAKE_RESP = 0x03,
    PACKET_TYPE_MESSAGE = 0x04
};

#pragma pack(push, 1)

struct MeshPacketHeader {
    uint8_t type;
    uint16_t senderId; // Derived from LoRa address or unique ID
};

struct BeaconPacket {
    MeshPacketHeader header;
    char name[10]; // Short username
};

struct HandshakePacket {
    MeshPacketHeader header;
    uint8_t publicKey[64]; // ECDH Public Key (example size, usually 64 bytes for secp256r1 uncompressed)
};

struct MessagePacket {
    MeshPacketHeader header;
    uint8_t iv[16]; // AES IV
    uint16_t payloadLength;
    uint8_t encryptedPayload[200]; // Max payload size
};

#pragma pack(pop)

#endif // PACKET_TYPES_H
