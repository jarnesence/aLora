#include "MeshManager.h"
#include "../config.h"
#include <SPI.h>
#include <RadioLib.h>

MeshManager* MeshManager::instance = nullptr;

MeshManager::MeshManager() : radio(LoraMesher::getInstance()) {
    instance = this;
    peerCount = 0;
    onMessageReceived = nullptr;
    onPairingRequest = nullptr;
    peerMutex = xSemaphoreCreateMutex();
    cryptoMutex = xSemaphoreCreateMutex();
}

void MeshManager::processReceivedPackets(void* parameters) {
    for (;;) {
        ulTaskNotifyTake(pdPASS, portMAX_DELAY);

        if (instance) {
            while (instance->radio.getReceivedQueueSize() > 0) {
                AppPacket<uint8_t>* packet = instance->radio.getNextAppPacket<uint8_t>();
                instance->processPacket(packet);
                instance->radio.deletePacket(packet);
            }
        }
    }
}

void MeshManager::init(const char* name) {
    strncpy(localName, name, 9);
    localName[9] = '\0';

    Serial.println("Initializing Crypto...");
    CryptoManager::init();
    CryptoManager::generateKeyPair(localPublicKey, localPrivateKey);
    Serial.println("Crypto initialized");

    // Initialize LoRaMesher with explicit configuration
    Serial.println("Starting LoRaMesher initialization...");
    Serial.printf("LoRa Pins: CS=%d, RST=%d, IRQ=%d\n", 
                  PIN_LORA_CS, PIN_LORA_RST, PIN_LORA_DIO0);
    Serial.printf("SPI Pins: SCK=%d, MISO=%d, MOSI=%d\n", 
                  PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI);
    Serial.printf("LoRa Config: FREQ=%.1f MHz, BW=%.1f kHz, SF=%d, CR=%d, POWER=%d dBm\n", 
                  LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, LORA_POWER);
    
    // Initialize SPI with custom pins
    SPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_CS);
    Serial.println("SPI initialized");
    
    // Create LoRaMesher configuration
    LoraMesher::LoraMesherConfig config;
    config.loraCs = PIN_LORA_CS;
    config.loraRst = PIN_LORA_RST;
    config.loraIrq = PIN_LORA_DIO0;
    config.loraIo1 = -1;  // Not used for SX1278
    config.module = LoraMesher::LoraModules::SX1278_MOD;
    config.freq = LORA_FREQ;  // 433.0 MHz
    config.bw = LORA_BW;  // 125.0 kHz
    config.sf = LORA_SF;  // Spreading Factor 9
    config.cr = LORA_CR;  // Coding Rate 7
    config.power = LORA_POWER;  // 20 dBm
    config.syncWord = LORA_SYNC_WORD;  // 0x12
    config.spi = &SPI;  // Pass SPI object pointer
    
    // Create task for receiving packets
    TaskHandle_t receiveLoRaMessage_Handle = NULL;
    xTaskCreate(
        processReceivedPackets,
        "Receive App Data",
        4096,
        NULL,
        2,
        &receiveLoRaMessage_Handle);

    radio.setReceiveAppDataTaskHandle(receiveLoRaMessage_Handle);
    
    // Initialize and start LoRaMesher with config
    Serial.println("Calling radio.begin() with config...");
    radio.begin(config);
    Serial.println("LoRa module initialized");
    
    // Start the mesh network
    Serial.println("Starting mesh network...");
    radio.start();
    Serial.println("LoRa mesh started");

    localId = radio.getLocalAddress();
    Serial.printf("LoRa Initialized. Local ID: 0x%04X\n", localId);
}

void MeshManager::update() {
    static unsigned long lastBeacon = 0;
    if (millis() - lastBeacon > 10000) { // Every 10 seconds
        broadcastBeacon();
        lastBeacon = millis();
    }
}

void MeshManager::processPacket(AppPacket<uint8_t>* packet) {
    if (packet->getPayloadLength() < sizeof(MeshPacketHeader)) return;

    MeshPacketHeader* header = (MeshPacketHeader*)packet->payload;
    uint16_t from = packet->src;

    switch (header->type) {
        case PACKET_TYPE_BEACON:
            if (packet->getPayloadLength() >= sizeof(BeaconPacket))
                handleBeacon(from, (BeaconPacket*)packet->payload);
            break;
        case PACKET_TYPE_HANDSHAKE_REQ:
            if (packet->getPayloadLength() >= sizeof(HandshakePacket))
                handleHandshakeReq(from, (HandshakePacket*)packet->payload);
            break;
        case PACKET_TYPE_HANDSHAKE_RESP:
            if (packet->getPayloadLength() >= sizeof(HandshakePacket))
                handleHandshakeResp(from, (HandshakePacket*)packet->payload);
            break;
        case PACKET_TYPE_MESSAGE:
            if (packet->getPayloadLength() >= sizeof(MessagePacket))
                handleMessage(from, (MessagePacket*)packet->payload);
            break;
    }
}

void MeshManager::broadcastBeacon() {
    BeaconPacket bp;
    bp.header.type = PACKET_TYPE_BEACON;
    bp.header.senderId = localId;
    strncpy(bp.name, localName, 9);
    bp.name[9] = '\0';

    radio.sendReliablePacket(BROADCAST_ADDR, (uint8_t*)&bp, sizeof(BeaconPacket));
}

void MeshManager::handleBeacon(uint16_t from, BeaconPacket* p) {
    xSemaphoreTake(peerMutex, portMAX_DELAY);
    MeshPeer* peer = findPeer(from);
    if (!peer) {
        if (peerCount < MAX_PEERS) {
            peer = &peers[peerCount++];
            peer->id = from;
            peer->status = PEER_DISCOVERED;
        }
    }
    if (peer) {
        strncpy(peer->name, p->name, 10);
        peer->lastSeen = millis();
    }
    xSemaphoreGive(peerMutex);
}

void MeshManager::initiateHandshake(uint16_t peerId) {
    xSemaphoreTake(peerMutex, portMAX_DELAY);
    MeshPeer* peer = findPeer(peerId);
    if (!peer) {
        xSemaphoreGive(peerMutex);
        return;
    }

    HandshakePacket hp;
    hp.header.type = PACKET_TYPE_HANDSHAKE_REQ;
    hp.header.senderId = localId;
    memcpy(hp.publicKey, localPublicKey, 64);

    radio.sendReliablePacket(peerId, (uint8_t*)&hp, sizeof(HandshakePacket));
    peer->status = PEER_HANDSHAKE_SENT;
    xSemaphoreGive(peerMutex);
}

void MeshManager::handleHandshakeReq(uint16_t from, HandshakePacket* p) {
    xSemaphoreTake(peerMutex, portMAX_DELAY);
    MeshPeer* peer = findPeer(from);
    if (!peer) {
        if (peerCount < MAX_PEERS) {
            peer = &peers[peerCount++];
            peer->id = from;
            strcpy(peer->name, "Unknown");
        }
    }

    if (peer) {
        if (CryptoManager::computeSharedSecret(p->publicKey, localPrivateKey, peer->sharedSecret)) {
             peer->status = PEER_HANDSHAKE_RECEIVED;
        }
    }
    xSemaphoreGive(peerMutex);

    if (peer && onPairingRequest) {
        onPairingRequest(from);
    }
}

void MeshManager::acceptHandshake(uint16_t peerId) {
    xSemaphoreTake(peerMutex, portMAX_DELAY);
    MeshPeer* peer = findPeer(peerId);
    if (!peer) {
        xSemaphoreGive(peerMutex);
        return;
    }

    HandshakePacket hp;
    hp.header.type = PACKET_TYPE_HANDSHAKE_RESP;
    hp.header.senderId = localId;
    memcpy(hp.publicKey, localPublicKey, 64);

    radio.sendReliablePacket(peerId, (uint8_t*)&hp, sizeof(HandshakePacket));
    peer->status = PEER_PAIRED;
    xSemaphoreGive(peerMutex);
}

void MeshManager::handleHandshakeResp(uint16_t from, HandshakePacket* p) {
    xSemaphoreTake(peerMutex, portMAX_DELAY);
    MeshPeer* peer = findPeer(from);
    if (peer) {
        if (CryptoManager::computeSharedSecret(p->publicKey, localPrivateKey, peer->sharedSecret)) {
            peer->status = PEER_PAIRED;
        }
    }
    xSemaphoreGive(peerMutex);
}

void MeshManager::sendMessage(uint16_t peerId, const char* text) {
    uint8_t secret[32];
    bool found = false;

    xSemaphoreTake(peerMutex, portMAX_DELAY);
    MeshPeer* peer = findPeer(peerId);
    if (peer && peer->status == PEER_PAIRED) {
        memcpy(secret, peer->sharedSecret, 32);
        found = true;
    }
    xSemaphoreGive(peerMutex);

    if (!found) return;

    MessagePacket mp;
    mp.header.type = PACKET_TYPE_MESSAGE;
    mp.header.senderId = localId;

    CryptoManager::generateRandomIV(mp.iv);

    size_t len = strlen(text);
    if (len > 150) len = 150;

    size_t outLen;

    // Encrypt with crypto lock
    xSemaphoreTake(cryptoMutex, portMAX_DELAY);
    CryptoManager::encrypt((uint8_t*)text, len, secret, mp.iv, mp.encryptedPayload, outLen);
    xSemaphoreGive(cryptoMutex);

    mp.payloadLength = outLen;

    radio.sendReliablePacket(peerId, (uint8_t*)&mp, sizeof(MessagePacket));
}

void MeshManager::handleMessage(uint16_t from, MessagePacket* p) {
    uint8_t secret[32];
    bool found = false;

    xSemaphoreTake(peerMutex, portMAX_DELAY);
    MeshPeer* peer = findPeer(from);

    if (peer && peer->status == PEER_PAIRED) {
        memcpy(secret, peer->sharedSecret, 32);
        found = true;
    }
    xSemaphoreGive(peerMutex);

    if (!found) return;

    uint8_t decrypted[200];
    size_t outLen;

    // Decrypt with crypto lock
    xSemaphoreTake(cryptoMutex, portMAX_DELAY);
    CryptoManager::decrypt(p->encryptedPayload, p->payloadLength, secret, p->iv, decrypted, outLen);
    xSemaphoreGive(cryptoMutex);

    decrypted[outLen] = '\0';

    if (onMessageReceived) {
        onMessageReceived(from, (char*)decrypted);
    }
}

void MeshManager::setOnMessageReceived(void (*cb)(uint16_t, const char*)) {
    onMessageReceived = cb;
}

void MeshManager::setOnPairingRequest(void (*cb)(uint16_t)) {
    onPairingRequest = cb;
}

uint16_t MeshManager::getLocalId() {
    return localId;
}

int MeshManager::getPeerCount() {
    xSemaphoreTake(peerMutex, portMAX_DELAY);
    int c = peerCount;
    xSemaphoreGive(peerMutex);
    return c;
}

bool MeshManager::getPeer(int index, MeshPeer* outPeer) {
    bool ret = false;
    xSemaphoreTake(peerMutex, portMAX_DELAY);
    if (index >= 0 && index < peerCount) {
        memcpy(outPeer, &peers[index], sizeof(MeshPeer));
        ret = true;
    }
    xSemaphoreGive(peerMutex);
    return ret;
}

MeshPeer* MeshManager::findPeer(uint16_t id) {
    // Expected to be called with mutex held
    for (int i = 0; i < peerCount; i++) {
        if (peers[i].id == id) return &peers[i];
    }
    return nullptr;
}
