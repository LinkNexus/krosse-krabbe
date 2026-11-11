#include "MovementController.h"

bool MovementController::begin() {
  if (!m_motorManager.begin()) {
    return false;
  }
  m_motorManager.goToDefaultPosition();
  return true;
}

void MovementController::handleCommand(Button command) {
  if (!isMovementCommand(command)) {
    reset();
    return;
  }

  auto now = std::chrono::steady_clock::now();
  if (now < m_stepDeadline) {
    return; // der vorherige Schritt laeuft noch
  }

  for (const MotorCommand &cmd : nextStep(command)) {
    m_motorManager.execute(cmd);
  }

  // Der Oberschenkel legt pro Schritt den weitesten Weg zurueck (von einer
  // Schwungseite auf die andere); daraus ergibt sich die Schrittdauer.
  constexpr float maxTravelDegrees = 2.0f * HIP_SWING_DELTA;
  constexpr auto stepDuration = std::chrono::milliseconds(
      static_cast<int>(maxTravelDegrees / DEFAULT_SPEED * 1000.0f));
  m_stepDeadline = now + stepDuration;
}

void MovementController::update() { m_motorManager.update(); }

void MovementController::reset() {
  m_phaseA = true;
  m_stepDeadline = {};
}

MotorManager &MovementController::motorManager() { return m_motorManager; }

bool MovementController::isMovementCommand(Button command) {
  return command == Button::Forward || command == Button::Backward ||
         command == Button::RotateLeft || command == Button::RotateRight;
}

bool MovementController::isLeftLeg(LegId leg) {
  return leg == FrontLeft || leg == BackLeft;
}

bool MovementController::isGroupA(LegId leg) {
  return leg == FrontRight || leg == BackLeft;
}

float MovementController::travelIntent(LegId leg, Button command) {
  bool left = isLeftLeg(leg);

  switch (command) {
  case Button::Forward:
    return 1.0f;
  case Button::Backward:
    return -1.0f;
  case Button::RotateRight: // rechte Beine nach hinten, linke nach vorne ->
                            // Drehung im Uhrzeigersinn
    return left ? 1.0f : -1.0f;
  case Button::RotateLeft:
    return left ? -1.0f : 1.0f;
  default:
    return 0.0f;
  }
}

std::vector<MotorCommand> MovementController::nextStep(Button command) {
  if (!isMovementCommand(command)) {
    return {};
  }

  std::vector<MotorCommand> commands;
  commands.reserve(8); // 4 Beine * 2 Gelenke

  for (uint8_t i = 0; i < 4; ++i) {
    LegId leg = static_cast<LegId>(i);

    bool swinging = (isGroupA(leg) == m_phaseA);
    float role = swinging
                     ? 1.0f
                     : -1.0f; // Schwungphase: Bein bewegt sich in Laufrichtung;
                              // Standphase: Bein schiebt entgegengesetzt
    float mechMirror =
        isLeftLeg(leg)
            ? -1.0f
            : 1.0f; // linke/rechte Beine sind spiegelverkehrt montiert

    float hipAngle = m_motorManager.getDefaultAngle(leg, LegType::UpperLeg) + mechMirror * travelIntent(leg, command) *
                                         role * HIP_SWING_DELTA;
    float kneeAngle = swinging ? KNEE_LIFTED : KNEE_GROUNDED;

    commands.push_back({leg, UpperLeg, hipAngle, DEFAULT_SPEED});
    commands.push_back({leg, LowerLeg, kneeAngle, DEFAULT_SPEED});
  }

  m_phaseA = !m_phaseA;
  return commands;
}
