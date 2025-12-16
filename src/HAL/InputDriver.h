#ifndef INPUT_DRIVER_H
#define INPUT_DRIVER_H

enum InputEvent {
    EVENT_NONE,
    EVENT_NEXT,
    EVENT_PREV,
    EVENT_SELECT,
    EVENT_LONG_PRESS
};

class InputDriver {
public:
    virtual ~InputDriver() {}

    virtual void init() = 0;
    virtual void update() = 0; // Polling or processing method
    virtual InputEvent getEvent() = 0; // Retrieve last event
};

#endif // INPUT_DRIVER_H
