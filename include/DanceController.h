#pragma once

#include "Leg.h"
#include "MotorManager.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>

/// Spielt nach einem einzelnen Startsignal genau eine Tanzsequenz ab.
///
/// Die Oberschenkel-Ziele sind Offsets zu ihren individuellen Default-Winkeln.
/// Die Unterschenkel verwenden absolute Tanzwinkel, weil ihr Defaultwinkel
/// keine Standposition ist. Der Controller bleibt bis zum rechnerischen Ende
/// der Servo-Interpolation fuer die abschliessende Standpose aktiv. Die Servos
/// besitzen keine Positionsrueckmeldung.
class DanceController {
public:
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;

  explicit DanceController(MotorManager &motorManager);

  /// Startet den Tanz einmalig. Gibt false zurueck, falls bereits ein Tanz
  /// laeuft oder der Controller nach einem Motorfehler gesperrt ist.
  bool start();

  /// Treibt die nicht blockierende Tanz-Zustandsmaschine weiter. Muss im
  /// Hauptloop zusammen mit MotorManager::update() aufgerufen werden.
  void update(TimePoint now = Clock::now());

  /// Bleibt bis nach der rechnerisch abgeschlossenen Abschluss-Pose true. In
  /// dieser Zeit darf der Hauptloop keine anderen Bewegungsbefehle ausfuehren.
  bool isActive() const;

  /// Ein Fehler sperrt weitere Bewegungsbefehle, damit keine nur teilweise
  /// ausgefuehrte Pose als sicherer Abschluss behandelt wird.
  bool hasFault() const;

private:
  enum class State : uint8_t {
    Idle,
    Moving,
    Holding,
    ReturningAfterFault,
    Fault,
  };

  struct DancePose {
    float lowerLegSideOffset;
    float lowerLegAngle;
    std::chrono::milliseconds holdAfterArrival;
  };

  bool playPose(std::size_t index);
  void enterFault();
  static bool isLeftLeg(LegId leg);

  MotorManager &m_motorManager;
  State m_state = State::Idle;
  std::size_t m_poseIndex = 0;
  std::chrono::steady_clock::time_point m_holdDeadline{};

  // -----------------------------------------------------------------------
  // TANZ-KALIBRIERUNG
  //
  // Oberschenkel: individueller Defaultwinkel.
  // Unterschenkel: absolute Winkel fuer Stand, Hocke und Strecken. Beim
  // seitlichen Wiegen wird der Seiten-Offset links/rechts gegensinnig addiert.
  // Beim Kalibrieren nur die Werte in diesem Block anpassen.
  // -----------------------------------------------------------------------

  static constexpr float STAND_LOWER_LEG_ANGLE_DEGREES = 20.0f;
  static constexpr float CROUCH_LOWER_LEG_ANGLE_DEGREES = 30.0f;
  static constexpr float STRETCH_LOWER_LEG_ANGLE_DEGREES = 10.0f;
  static constexpr float SWAY_LOWER_LEG_OFFSET_DEGREES = 15.0f;

  // Langsamer, konservativer Startwert. Der MotorManager erlaubt derzeit
  // hoechstens 70 Grad/Sekunde.
  static constexpr float DANCE_SPEED_DEGREES_PER_SECOND = 30.0f;

  // Diese Zeiten beginnen erst, nachdem alle acht Servos ihre Zielwinkel
  // rechnerisch vollstaendig erreicht haben.
  // Beim ersten PWM-Ziel kennt der Treiber die echte Servoposition noch
  // nicht. Diese zusaetzliche, leicht kalibrierbare Startreserve verhindert,
  // dass die Hocke schon waehrend der physischen Standfahrt beginnt.
  static constexpr uint16_t START_STAND_SETTLE_MS = 1500;
  static constexpr uint16_t CROUCH_HOLD_MS = 300;
  static constexpr uint16_t STRETCH_HOLD_MS = 300;
  static constexpr uint16_t SWAY_HOLD_MS = 300;
  static constexpr uint16_t BETWEEN_MOVES_HOLD_MS = 150;
  static constexpr uint16_t FINISH_HOLD_MS = 300;

  static const std::array<DancePose, 13> POSES;
};
