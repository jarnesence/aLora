#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "../HAL/DisplayDriver.h"
#include "../HAL/InputDriver.h"
#include "../Network/MeshManager.h"

enum UIState {
    STATE_LIST_PEERS,
    STATE_CHAT_VIEW,
    STATE_KEYBOARD,
    STATE_PAIRING_REQUEST
};

class UIManager {
private:
    DisplayDriver* display;
    InputDriver* input;
    MeshManager* mesh;

    UIState currentState;

    // Peer List State
    int selectedPeerIndex;

    // Chat State
    uint16_t currentChatPeerId;
    char chatHistory[5][20]; // Last 5 messages, 20 chars max for display
    int chatHistoryCount;

    // Keyboard State
    char inputText[50];
    int inputCursor;
    int kbdX, kbdY; // Grid position
    static const char* kbdLayout[4]; // 4 rows

    // Pairing Request State
    uint16_t pairingRequestId;

    // Helpers
    void drawPeerList();
    void drawChat();
    void drawKeyboard();
    void drawPairingRequest();

    void handleInputPeerList(InputEvent e);
    void handleInputChat(InputEvent e);
    void handleInputKeyboard(InputEvent e);
    void handleInputPairing(InputEvent e);

public:
    UIManager(DisplayDriver* disp, InputDriver* inp, MeshManager* m);
    void init();
    void update();

    // Callbacks from Mesh
    void onMessageReceived(uint16_t from, const char* msg);
    void onPairingRequest(uint16_t from);
};

#endif // UI_MANAGER_H
