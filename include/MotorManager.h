#pragma once
#include "MotorCommand.h"
#include "Leg.h"
#include "PCA9685Interface.h"

class MotorManager {
    public:
    MotorManager();

    bool begin(); 
    bool isI2cConnected();
    bool setSpeed(float degreesPerSecond); // returns true if speed is accepted 
    float getDefaultAngle(LegId leg, LegType type);
    float getSpeed();

    bool canExecute(const MotorCommand& cmdToExecute) const;
    bool execute(MotorCommand cmdToExecute);
    void goToDefaultPosition(); // safe start position, has to be done
    void stop(); // stop all Motors at current Angle
    void update();
    bool isMoving() const;

    private:
    void init(); // puts pins in m_legs 

    float m_currentSpeed = 2;
    float m_maxSpeed = 70; // <-- aktuell Random
    bool m_wasStarted = false; // becomes true after goToDefaultPosition()
    PCA9685Interface m_motorDriver;
    Leg m_legs[4]; // <--- id wie in Discord -1

    // ---- MOTOR PINS ----
    const uint8_t m_oberschenkel_1 = 0;
    const uint8_t m_unterschenkel_1 = 1;
    const uint8_t m_oberschenkel_2 = 2;
    const uint8_t m_unterschenkel_2 = 3;
    const uint8_t m_oberschenkel_3 = 4;
    const uint8_t m_unterschenkel_3 = 5;
    const uint8_t m_oberschenkel_4 = 6;
    const uint8_t m_unterschenkel_4 = 7;
};
