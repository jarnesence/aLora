#include "UIManager.h"

const char* UIManager::kbdLayout[4] = {
    "1234567890",
    "QWERTYUIOP",
    "ASDFGHJKL",
    "ZXCVBNM <OK" // < is backspace, OK is enter
};

UIManager::UIManager(DisplayDriver* disp, InputDriver* inp, MeshManager* m)
    : display(disp), input(inp), mesh(m) {
    currentState = STATE_LIST_PEERS;
    selectedPeerIndex = 0;
    inputCursor = 0;
    kbdX = 0;
    kbdY = 0;
    memset(inputText, 0, sizeof(inputText));
    chatHistoryCount = 0;
}

void UIManager::init() {
    display->init();
    input->init();
    display->clear();
    display->print("Initializing...");
    display->display();
}

void UIManager::update() {
    input->update();
    InputEvent e = input->getEvent();

    display->clear();

    switch (currentState) {
        case STATE_LIST_PEERS:
            handleInputPeerList(e);
            drawPeerList();
            break;
        case STATE_CHAT_VIEW:
            handleInputChat(e);
            drawChat();
            break;
        case STATE_KEYBOARD:
            handleInputKeyboard(e);
            drawKeyboard();
            break;
        case STATE_PAIRING_REQUEST:
            handleInputPairing(e);
            drawPairingRequest();
            break;
    }

    display->display();
}

void UIManager::drawPeerList() {
    display->setCursor(0, 0);
    display->setTextSize(1);
    display->setTextColor(1); // WHITE
    display->println("Peers:");

    int count = mesh->getPeerCount();
    if (count == 0) {
        display->println("No peers found.");
    } else {
        for (int i = 0; i < count; i++) {
            if (i == selectedPeerIndex) {
                display->setTextColor(0, 1); // Invert
            } else {
                display->setTextColor(1, 0);
            }
            MeshManager::MeshPeer p;
            if (mesh->getPeer(i, &p)) {
                display->print(p.name);
                if (p.status == PEER_PAIRED) display->print(" [P]");
                display->println("");
            }
        }
    }
}

void UIManager::handleInputPeerList(InputEvent e) {
    int count = mesh->getPeerCount();
    if (e == EVENT_NEXT) {
        selectedPeerIndex++;
        if (selectedPeerIndex >= count) selectedPeerIndex = 0;
    } else if (e == EVENT_PREV) {
        selectedPeerIndex--;
        if (selectedPeerIndex < 0) selectedPeerIndex = count - 1;
    } else if (e == EVENT_SELECT) {
        if (count > 0) {
            MeshManager::MeshPeer p;
            if (mesh->getPeer(selectedPeerIndex, &p)) {
                if (p.status == PEER_PAIRED) {
                    currentChatPeerId = p.id;
                    currentState = STATE_CHAT_VIEW;
                } else {
                    // Initiate handshake
                    mesh->initiateHandshake(p.id);
                    // Notify user
                    display->clear();
                    display->println("Handshake Sent...");
                    display->display();
                    delay(1000);
                }
            }
        }
    }
}

void UIManager::drawChat() {
    display->setCursor(0, 0);
    display->setTextSize(1);
    display->setTextColor(1);
    display->print("Chat: ");
    display->println(currentChatPeerId);

    // Draw history
    for (int i = 0; i < chatHistoryCount; i++) {
        display->println(chatHistory[i]);
    }

    display->println("--------------");
    display->println("[Click to Reply]");
}

void UIManager::handleInputChat(InputEvent e) {
    if (e == EVENT_SELECT) {
        currentState = STATE_KEYBOARD;
        memset(inputText, 0, sizeof(inputText));
        inputCursor = 0;
    } else if (e == EVENT_LONG_PRESS) {
        currentState = STATE_LIST_PEERS;
    }
}

void UIManager::drawKeyboard() {
    // Show text field
    display->setCursor(0, 0);
    display->setTextSize(1);
    display->setTextColor(1);
    display->println(inputText);

    // Draw grid
    int startY = 20;
    int charW = 10;
    int charH = 10;

    for (int r = 0; r < 4; r++) {
        const char* row = kbdLayout[r];
        int len = strlen(row);

        int x = 0;
        for (int c = 0; c < len; c++) {
            bool selected = (r == kbdY && c == kbdX);

            char ch = row[c];
            if (selected) display->setTextColor(0, 1);
            else display->setTextColor(1, 0);

            display->setCursor(x * charW, startY + r * charH);
            display->print(ch);

            x++;
        }
    }
}

void UIManager::handleInputKeyboard(InputEvent e) {
    if (e == EVENT_NEXT) {
        kbdX++;
        if (kbdX >= 10) {
            kbdX = 0;
            kbdY++;
            if (kbdY >= 4) kbdY = 0;
        }
    } else if (e == EVENT_PREV) {
        kbdX--;
        if (kbdX < 0) {
            kbdX = 9;
            kbdY--;
            if (kbdY < 0) kbdY = 3;
        }
    } else if (e == EVENT_SELECT) {
        char rowContent[15];
        strcpy(rowContent, kbdLayout[kbdY]);
        if (kbdX < strlen(rowContent)) {
            char ch = rowContent[kbdX];

            if (ch == '<') {
                if (inputCursor > 0) inputText[--inputCursor] = 0;
            } else if (ch == 'O' && rowContent[kbdX+1] == 'K') {
                mesh->sendMessage(currentChatPeerId, inputText);
                currentState = STATE_CHAT_VIEW;
            } else {
                if (inputCursor < 49) {
                    inputText[inputCursor++] = ch;
                    inputText[inputCursor] = 0;
                }
            }
        }
    }
}

void UIManager::drawPairingRequest() {
    display->setCursor(0, 0);
    display->setTextSize(1);
    display->setTextColor(1);
    display->println("Pairing Request!");
    display->print("From: ");
    display->println(pairingRequestId);
    display->println("Accept?");
    display->println("[Click] Yes");
    display->println("[Long] Ignore");
}

void UIManager::handleInputPairing(InputEvent e) {
    if (e == EVENT_SELECT) {
        mesh->acceptHandshake(pairingRequestId);
        currentState = STATE_LIST_PEERS;
        // Optionally show success message
    } else if (e == EVENT_LONG_PRESS) {
        currentState = STATE_LIST_PEERS;
    }
}

void UIManager::onMessageReceived(uint16_t from, const char* msg) {
    if (chatHistoryCount < 5) {
        strncpy(chatHistory[chatHistoryCount++], msg, 19);
    } else {
        for (int i = 0; i < 4; i++) strcpy(chatHistory[i], chatHistory[i+1]);
        strncpy(chatHistory[4], msg, 19);
    }
}

void UIManager::onPairingRequest(uint16_t from) {
    pairingRequestId = from;
    currentState = STATE_PAIRING_REQUEST;
}
