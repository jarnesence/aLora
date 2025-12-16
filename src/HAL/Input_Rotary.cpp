#include "Input_Rotary.h"

// Global pointer for ISR
static AiEsp32RotaryEncoder* _globalRotary = nullptr;

void IRAM_ATTR Input_Rotary::readEncoderISR() {
    if (_globalRotary) {
        _globalRotary->readEncoder_ISR();
    }
}

Input_Rotary::Input_Rotary(int a, int b, int btn)
    : pinA(a), pinB(b), pinBtn(btn), lastBtnPress(0), pendingEvent(EVENT_NONE) {
    rotaryEncoder = new AiEsp32RotaryEncoder(pinA, pinB, pinBtn, -1, 4); // steps=4
    _globalRotary = rotaryEncoder;
}

Input_Rotary::~Input_Rotary() {
    delete rotaryEncoder;
}

void Input_Rotary::init() {
    rotaryEncoder->begin();
    rotaryEncoder->setup(readEncoderISR);
    // Use a very large range to simulate infinite scrolling.
    // We check delta, so absolute value doesn't matter much as long as it doesn't hit the wall.
    // -100000 to 100000 is plenty.
    rotaryEncoder->setBoundaries(-100000, 100000, false);
    rotaryEncoder->setAcceleration(250);
}

void Input_Rotary::update() {
    static long lastEncoderValue = 0;

    // Check rotation
    if (rotaryEncoder->encoderChanged()) {
        long currentValue = rotaryEncoder->readEncoder();

        if (currentValue > lastEncoderValue) {
            pendingEvent = EVENT_NEXT;
        } else if (currentValue < lastEncoderValue) {
            pendingEvent = EVENT_PREV;
        }

        lastEncoderValue = currentValue;
    }

    // Check button
    if (rotaryEncoder->isEncoderButtonClicked()) {
        pendingEvent = EVENT_SELECT;
    }
}

InputEvent Input_Rotary::getEvent() {
    InputEvent e = pendingEvent;
    pendingEvent = EVENT_NONE;
    return e;
}
