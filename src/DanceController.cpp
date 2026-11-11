#include "DanceController.h"

#include <array>
#include <iostream>

const std::array<DanceController::DancePose, 13> DanceController::POSES = {{
    // Erst die normale Standposition herstellen.
    {0.0f, STAND_LOWER_LEG_ANGLE_DEGREES,
     std::chrono::milliseconds(START_STAND_SETTLE_MS)},

    // Hocke und Strecken, jeweils mit der Standpose als Zwischenpose.
    {0.0f, CROUCH_LOWER_LEG_ANGLE_DEGREES,
     std::chrono::milliseconds(CROUCH_HOLD_MS)},
    {0.0f, STAND_LOWER_LEG_ANGLE_DEGREES,
     std::chrono::milliseconds(BETWEEN_MOVES_HOLD_MS)},
    {0.0f, STRETCH_LOWER_LEG_ANGLE_DEGREES,
     std::chrono::milliseconds(STRETCH_HOLD_MS)},
    {0.0f, STAND_LOWER_LEG_ANGLE_DEGREES,
     std::chrono::milliseconds(BETWEEN_MOVES_HOLD_MS)},

    // Seitlich wiegen: eine Seite hockt, die andere streckt sich.
    {SWAY_LOWER_LEG_OFFSET_DEGREES, STAND_LOWER_LEG_ANGLE_DEGREES,
     std::chrono::milliseconds(SWAY_HOLD_MS)},
    {0.0f, STAND_LOWER_LEG_ANGLE_DEGREES,
     std::chrono::milliseconds(BETWEEN_MOVES_HOLD_MS)},
    {-SWAY_LOWER_LEG_OFFSET_DEGREES, STAND_LOWER_LEG_ANGLE_DEGREES,
     std::chrono::milliseconds(SWAY_HOLD_MS)},

    // Den gleichen Links-Rechts-Ablauf ein zweites Mal wiederholen.
    {0.0f, STAND_LOWER_LEG_ANGLE_DEGREES,
     std::chrono::milliseconds(BETWEEN_MOVES_HOLD_MS)},
    {SWAY_LOWER_LEG_OFFSET_DEGREES, STAND_LOWER_LEG_ANGLE_DEGREES,
     std::chrono::milliseconds(SWAY_HOLD_MS)},
    {0.0f, STAND_LOWER_LEG_ANGLE_DEGREES,
     std::chrono::milliseconds(BETWEEN_MOVES_HOLD_MS)},
    {-SWAY_LOWER_LEG_OFFSET_DEGREES, STAND_LOWER_LEG_ANGLE_DEGREES,
     std::chrono::milliseconds(SWAY_HOLD_MS)},

    // Erst nach dem rechnerischen Abschluss dieser Standpose endet der Tanz
    // und der Hauptloop darf neue Befehle annehmen.
    {0.0f, STAND_LOWER_LEG_ANGLE_DEGREES,
     std::chrono::milliseconds(FINISH_HOLD_MS)},
}};

DanceController::DanceController(MotorManager &motorManager)
    : m_motorManager(motorManager) {}

bool DanceController::isLeftLeg(LegId leg) {
  return leg == FrontLeft || leg == BackLeft;
}

bool DanceController::start() {
  if (m_state != State::Idle) {
    return false;
  }

  m_poseIndex = 0;
  m_holdDeadline = {};

  if (!playPose(m_poseIndex)) {
    enterFault();
    return false;
  }

  m_state = State::Moving;
  return true;
}

void DanceController::update(TimePoint now) {
  switch (m_state) {
  case State::Idle:
  case State::Fault:
    return;

  case State::ReturningAfterFault:
    if (!m_motorManager.isMoving()) {
      m_state = State::Fault;
    }
    return;

  case State::Moving:
    if (m_motorManager.isMoving()) {
      return;
    }

    m_holdDeadline = now + POSES[m_poseIndex].holdAfterArrival;
    m_state = State::Holding;
    return;

  case State::Holding:
    if (now < m_holdDeadline) {
      return;
    }

    ++m_poseIndex;
    if (m_poseIndex >= POSES.size()) {
      m_state = State::Idle;
      m_poseIndex = 0;
      m_holdDeadline = {};
      return;
    }

    if (!playPose(m_poseIndex)) {
      enterFault();
      return;
    }

    m_state = State::Moving;
    return;
  }
}

bool DanceController::isActive() const { return m_state != State::Idle; }

bool DanceController::hasFault() const {
  return m_state == State::ReturningAfterFault || m_state == State::Fault;
}

bool DanceController::playPose(std::size_t index) {
  const DancePose &pose = POSES[index];
  std::array<MotorCommand, 8> commands{};
  std::size_t commandIndex = 0;

  for (uint8_t i = 0; i < 4; ++i) {
    const LegId leg = static_cast<LegId>(i);
    const float side = isLeftLeg(leg) ? 1.0f : -1.0f;

    const float upperLegAngle =
        m_motorManager.getDefaultAngle(leg, UpperLeg);
    const float lowerLegAngle =
        pose.lowerLegAngle + side * pose.lowerLegSideOffset;

    commands[commandIndex++] = {leg, UpperLeg, upperLegAngle,
                                DANCE_SPEED_DEGREES_PER_SECOND};
    commands[commandIndex++] = {leg, LowerLeg, lowerLegAngle,
                                DANCE_SPEED_DEGREES_PER_SECOND};
  }

  // Erst alle acht Ziele pruefen. So wird bei einer Fehlkalibrierung keine
  // asymmetrische, nur teilweise ausgefuehrte Pose gestartet.
  for (const MotorCommand &command : commands) {
    if (!m_motorManager.canExecute(command)) {
      return false;
    }
  }

  for (const MotorCommand &command : commands) {
    if (!m_motorManager.execute(command)) {
      return false;
    }
  }

  return true;
}

void DanceController::enterFault() {
  std::cerr << "Fehler: Tanzpose wurde vom MotorManager abgelehnt. "
               "Rueckkehr zur Standposition."
            << std::endl;
  if (playPose(0)) {
    m_state = State::ReturningAfterFault;
  } else {
    m_state = State::Fault;
  }
}
