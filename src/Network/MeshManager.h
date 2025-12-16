#ifndef MESH_MANAGER_H
#define MESH_MANAGER_H

#include <Arduino.h>
#include <LoRaMesher.h>
#include <freertos/semphr.h>
#include "PacketTypes.h"
#include "../Security/CryptoManager.h"

// Peer state
enum PeerStatus {
    PEER_UNKNOWN,
    PEER_DISCOVERED,
    PEER_HANDSHAKE_SENT,
    PEER_HANDSHAKE_RECEIVED, // Waiting for User Accept
    PEER_PAIRED
};

struct MeshPeer {
    uint16_t id;
    char name[10];
    PeerStatus status;
    uint8_t publicKey[64];
    uint8_t sharedSecret[32]; // AES-256 key
    uint32_t lastSeen;
};

class MeshManager {
private:
    static MeshManager* instance;
    LoRaMesher& radio;

    // Local Identity
    uint16_t localId;
    char localName[10];
    uint8_t localPublicKey[64];
    uint8_t localPrivateKey[32];

    // Peer Table
    static const int MAX_PEERS = 10;
    MeshPeer peers[MAX_PEERS];
    int peerCount;

    // Thread safety
    SemaphoreHandle_t peerMutex;
    SemaphoreHandle_t cryptoMutex; // Dedicated mutex for AES operations

    // Callbacks
    void (*onMessageReceived)(uint16_t from, const char* msg);
    void (*onPairingRequest)(uint16_t from);

    // Internal handlers
    void processPacket(AppPacket<uint8_t>* packet);
    void handleBeacon(uint16_t from, BeaconPacket* p);
    void handleHandshakeReq(uint16_t from, HandshakePacket* p);
    void handleHandshakeResp(uint16_t from, HandshakePacket* p);
    void handleMessage(uint16_t from, MessagePacket* p);

public:
    MeshManager();
    void init(const char* name);
    void update();

    void broadcastBeacon();
    void initiateHandshake(uint16_t peerId);
    void acceptHandshake(uint16_t peerId);
    void sendMessage(uint16_t peerId, const char* text);

    void setOnMessageReceived(void (*cb)(uint16_t, const char*));
    void setOnPairingRequest(void (*cb)(uint16_t));

    uint16_t getLocalId();

    // Thread-safe accessors
    int getPeerCount();
    // Returns true if peer found and copied
    bool getPeer(int index, MeshPeer* outPeer);

    // Internal helper (needs mutex held)
    MeshPeer* findPeer(uint16_t id);

    // Static callback for LoRaMesher
    static void processReceivedPackets(void*);
};

#endif // MESH_MANAGER_H
