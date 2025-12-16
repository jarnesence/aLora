#include <Arduino.h>
#include "config.h"
#include "HAL/Display_SSD1306.h"
#include "HAL/Input_Rotary.h"
#include "Network/MeshManager.h"
#include "UI/UIManager.h"
#include <NimBLEDevice.h>

// Global objects
Display_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, PIN_I2C_SDA, PIN_I2C_SCL, SCREEN_ADDRESS);
Input_Rotary input(PIN_ROT_A, PIN_ROT_B, PIN_ROT_BTN);
MeshManager meshManager;
UIManager uiManager(&display, &input, &meshManager);

// BLE State
bool deviceConnected = false;
NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pCharacteristic = nullptr;

class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        deviceConnected = true;
    };
    void onDisconnect(NimBLEServer* pServer) {
        deviceConnected = false;
    }
};

void setupBLE() {
    NimBLEDevice::init("SecureLoRaMesh");
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService* pService = pServer->createService("ABCD"); // Example UUID
    pCharacteristic = pService->createCharacteristic("1234", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);

    pService->start();
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID("ABCD");
    pAdvertising->start();
}

// Forward callback
void onMeshMessage(uint16_t from, const char* msg) {
    if (deviceConnected) {
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "%d:%s", from, msg);
        pCharacteristic->setValue(buffer);
        pCharacteristic->notify();
    } else {
        uiManager.onMessageReceived(from, msg);
    }
}

void onPairingRequest(uint16_t from) {
    uiManager.onPairingRequest(from);
}

void setup() {
    Serial.begin(115200);

    // Mesh Init
    meshManager.setOnMessageReceived(onMeshMessage);
    meshManager.setOnPairingRequest(onPairingRequest);
    meshManager.init("User1");

    // UI Init
    uiManager.init();

    // BLE Init
    setupBLE();
}

void loop() {
    meshManager.update();

    if (!deviceConnected) {
        uiManager.update();
    } else {
        // BLE Mode
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
             // Handle BLE input to mesh if needed
             // For example: "SEND:ID:MESSAGE"
             // Not fully specified, but structure allows it.
        }

        display.clear();
        display.setCursor(0,0);
        display.print("BLE Connected");
        display.display();
    }

    delay(10);
}
