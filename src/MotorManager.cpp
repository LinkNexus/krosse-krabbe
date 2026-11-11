#include "MotorManager.h"
#include <iostream>

namespace {
// Platzhalter-Limits, muessen pro Gelenk kalibriert werden
constexpr float MIN_ANGLE = 0.0f;
constexpr float MAX_ANGLE = 180.0f;
}

MotorManager::MotorManager() {
    m_motorDriver.setSpeed(static_cast<uint16_t>(m_currentSpeed));
    init();
}

void MotorManager::init() {
    m_legs[0].upperLeg.pin = m_oberschenkel_1;
    m_legs[0].lowerLeg.pin = m_unterschenkel_1;
    m_legs[0].id = FrontRight;
    m_legs[1].upperLeg.pin = m_oberschenkel_2;
    m_legs[1].lowerLeg.pin = m_unterschenkel_2;
    m_legs[1].id = BackRight;
    m_legs[2].upperLeg.pin = m_oberschenkel_3;
    m_legs[2].lowerLeg.pin = m_unterschenkel_3;
    m_legs[2].id = FrontLeft;
    m_legs[3].upperLeg.pin = m_oberschenkel_4;
    m_legs[3].lowerLeg.pin = m_unterschenkel_4;
    m_legs[3].id = BackLeft;

    for (Leg& leg : m_legs) {
        for (MotorState* motor : {&leg.upperLeg, &leg.lowerLeg}) {
            motor->targetAngle = motor->defaultAngle;
            motor->minAngle = MIN_ANGLE;
            motor->maxAngle = MAX_ANGLE;
        }
    }
}

bool MotorManager::begin() {
    if (!m_motorDriver.begin()) {
      std::cerr << "Fehler: PCA9685 wurde nicht gefunden." << std::endl;
      return false;
    }
    return true;
}

bool MotorManager::isI2cConnected() {
    return m_motorDriver.isConnected();
}

bool MotorManager::setSpeed(float degreesPerSecond) {
    if (degreesPerSecond > m_maxSpeed || degreesPerSecond <= 0) {
        return false;
    }
    m_motorDriver.setSpeed(static_cast<uint16_t>(degreesPerSecond));
    m_currentSpeed = degreesPerSecond;
    return true;
}

float MotorManager::getSpeed() {
    return m_currentSpeed;
}

float MotorManager::getDefaultAngle(LegId leg, LegType type) {
    if (type == LegType::UpperLeg) {
        return m_legs[leg].upperLeg.defaultAngle;
    }
    return m_legs[leg].lowerLeg.defaultAngle;
}

bool MotorManager::canExecute(const MotorCommand& cmdToExecute) const {
    if (!m_wasStarted) {
        return false; // erst goToDefaultPosition() ausfuehren
    }

    if (cmdToExecute.legToMove > BackLeft) {
        return false;
    }

    if (cmdToExecute.legType != UpperLeg && cmdToExecute.legType != LowerLeg) {
        return false;
    }

    if (cmdToExecute.speed > m_maxSpeed) {
        return false;
    }

    const Leg& leg = m_legs[cmdToExecute.legToMove];
    const MotorState& motor =
        (cmdToExecute.legType == UpperLeg) ? leg.upperLeg : leg.lowerLeg;
    return cmdToExecute.angleToMove >= motor.minAngle &&
           cmdToExecute.angleToMove <= motor.maxAngle;
}

bool MotorManager::execute(MotorCommand cmdToExecute) {
    if (!canExecute(cmdToExecute)) {
        return false;
    }

    Leg& leg = m_legs[cmdToExecute.legToMove];
    MotorState& motor =
        (cmdToExecute.legType == UpperLeg) ? leg.upperLeg : leg.lowerLeg;

    if (cmdToExecute.speed > 0) {
        // Befehls-Geschwindigkeit gilt nur fuer diese eine Bewegung
        m_motorDriver.setSpeed(static_cast<uint16_t>(cmdToExecute.speed));
    }
    m_motorDriver.moveToAngle(motor.pin,
                              static_cast<uint8_t>(cmdToExecute.angleToMove));
    if (cmdToExecute.speed > 0) {
        m_motorDriver.setSpeed(static_cast<uint16_t>(m_currentSpeed));
    }

    motor.targetAngle = cmdToExecute.angleToMove;
    return true;
}

void MotorManager::goToDefaultPosition() {
    m_wasStarted = true; // ab jetzt nimmt isSafe() Befehle an

    for (Leg& leg : m_legs) {
        MotorCommand upperCmd;
        upperCmd.legToMove = leg.id;
        upperCmd.legType = UpperLeg;
        upperCmd.angleToMove = leg.upperLeg.defaultAngle;

        MotorCommand lowerCmd;
        lowerCmd.legToMove = leg.id;
        lowerCmd.legType = LowerLeg;
        lowerCmd.angleToMove = leg.lowerLeg.defaultAngle;

        this->execute(upperCmd);
        this->execute(lowerCmd);
    }
}

void MotorManager::stop() {
    for (const Leg& leg : m_legs) {
        m_motorDriver.stop(leg.upperLeg.pin);
        m_motorDriver.stop(leg.lowerLeg.pin);
    }
}

void MotorManager::update() {
    m_motorDriver.update();
}

bool MotorManager::isMoving() const {
    for (const Leg& leg : m_legs) {
        if (m_motorDriver.isMoving(leg.upperLeg.pin) ||
            m_motorDriver.isMoving(leg.lowerLeg.pin)) {
            return true;
        }
    }

    return false;
}
