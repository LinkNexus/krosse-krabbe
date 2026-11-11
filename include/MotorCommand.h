#pragma once
#include "Leg.h"

struct MotorCommand{
    LegId legToMove;
    LegType legType;
    float angleToMove;
    float speed = 0; // 0 or less means standard speed
};
