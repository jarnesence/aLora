#ifndef INPUT_ROTARY_H
#define INPUT_ROTARY_H

#include "InputDriver.h"
#include <AiEsp32RotaryEncoder.h>

class Input_Rotary : public InputDriver {
private:
    AiEsp32RotaryEncoder* rotaryEncoder;
    int pinA;
    int pinB;
    int pinBtn;
    unsigned long lastBtnPress;
    InputEvent pendingEvent;

public:
    Input_Rotary(int a, int b, int btn);
    ~Input_Rotary();

    void init() override;
    void update() override;
    InputEvent getEvent() override;

    // Static ISR wrapper
    static void IRAM_ATTR readEncoderISR();
};

#endif // INPUT_ROTARY_H
