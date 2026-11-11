#pragma once
#include <stdint.h>

enum LegType : uint8_t {
    LowerLeg = 0,
    UpperLeg = 1
};

enum LegId : uint8_t { // siehe Discord -1
    FrontRight = 0,
    BackRight = 1,
    FrontLeft = 2,
    BackLeft = 3,
};

struct MotorState {
    uint8_t pin;
    float targetAngle;
    float minAngle;
    float maxAngle;
    float defaultAngle = 90; // <-- 90° PLACEHOLDER
};

struct Leg {
    MotorState upperLeg;
    MotorState lowerLeg;
    LegId id;
};
