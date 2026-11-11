#pragma once
#include "Button.h"
#include "MotorCommand.h"
#include "MotorManager.h"
#include <chrono>
#include <vector>

/// <summary>
/// Wandelt einen Bewegungsbefehl (z.B. Button::Forward) in eine Abfolge von
/// MotorCommands fuer die vier Beine um und fuehrt sie ueber den eigenen
/// MotorManager aus. Es wird ein einfacher 2-Phasen Diagonalgang (Trab)
/// verwendet: FrontRight+BackLeft schwingen gemeinsam, danach
/// FrontLeft+BackRight. Die Schritte sind zeitgesteuert: der naechste Schritt
/// wird erst ausgeloest, wenn der vorherige abgeschlossen sein muesste.
///
/// Alle Winkel-Konstanten sind grobe Schaetzwerte und muessen beim Testen am
/// echten Roboter kalibriert werden (Richtung der Motoren, Range,
/// Geschwindigkeit).
/// </summary>
class MovementController {
public:
  MovementController() = default;

  /// Initialisiert den MotorManager und faehrt die sichere Startposition an.
  /// Muss einmalig vor handleCommand()/update() aufgerufen werden.
  bool begin();

  /// Muss wiederholt aufgerufen werden (z.B. im Loop). Bei einem
  /// Bewegungsbefehl (Forward/Backward/RotateLeft/RotateRight) wird der
  /// naechste Gang-Schritt erzeugt und ueber den MotorManager ausgefuehrt,
  /// sobald der vorherige Schritt fertig ist. Bei Button::None (oder Dance-/
  /// Tracking-Modus, die anderswo behandelt werden) wird nur die Gang-Phase
  /// zurueckgesetzt.
  void handleCommand(Button command);

  /// Treibt die laufenden Servo-Bewegungen an, muss im Loop aufgerufen werden.
  void update();

  /// Setzt die Gang-Phase zurueck, z.B. beim Stehenbleiben oder Moduswechsel.
  void reset();

  /// Zugriff auf den MotorManager, z.B. fuer Geschwindigkeit oder Status.
  MotorManager &motorManager();

private:
  /// Liefert die MotorCommands fuer den naechsten Gang-Schritt und wechselt
  /// dabei die Gang-Phase.
  std::vector<MotorCommand> nextStep(Button command);

  static bool isMovementCommand(Button command);
  static bool isLeftLeg(LegId leg);
  static bool isGroupA(LegId leg); // Diagonalpaar FrontRight/BackLeft
  static float
  travelIntent(LegId leg,
               Button command); // gewuenschte Laufrichtung des Beins: +1 vorne,
                                // -1 hinten, 0 keine

  MotorManager m_motorManager;
  bool m_phaseA = true; // welches Diagonalpaar gerade in der Schwungphase ist
  std::chrono::steady_clock::time_point
      m_stepDeadline{}; // fruehester Zeitpunkt fuer den naechsten Schritt

  // ---- Default-Winkel (Schaetzwerte, Kalibrierung noetig) ----
  static constexpr float NEUTRAL_ANGLE = 90.0f;
  static constexpr float KNEE_LIFTED =
      60.0f; // Unterschenkel: Bein angehoben (Schwungphase)
  static constexpr float KNEE_GROUNDED =
      45.0f; // Unterschenkel: Bein am Boden (Standphase)
  static constexpr float HIP_SWING_DELTA =
      25.0f; // Oberschenkel: wie weit das Bein vor/zurueck schwingt
  static constexpr float DEFAULT_SPEED =
      20.0f; // Grad/Sekunde; darf die MaxSpeed des MotorManagers nicht
             // ueberschreiten, sonst wird der Befehl abgelehnt
};
