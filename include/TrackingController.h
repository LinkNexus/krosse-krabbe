#pragma once
#include "BallPosition.h"
#include "Button.h"
#include "IBallDetector.h"
#include "MovementController.h"

/// <summary>
/// Verfolgungsmodus (Basislogik ohne Bildverarbeitung): fragt pro
/// Loop-Durchlauf die Ballposition ueber IBallDetector ab und wandelt sie in
/// einen Bewegungsbefehl um. Der Befehl wird ueber den vorhandenen
/// MovementController ausgefuehrt, damit der Verfolgungsmodus denselben Gang
/// (2-Phasen-Trab) nutzt wie der manuelle Modus.
///
/// Entscheidungslogik (Technische Spezifikation, Funktion 4, T5):
///  - Center:     vorwaerts
///  - Left/Right: Drehung zur jeweiligen Seite, bis der Ball zentriert ist
///  - NotVisible: Suchdrehung, bis der Ball wieder gefunden wird (in den
///                Anforderungen nicht spezifiziert, siehe SEARCH_DIRECTION)
///  - Fehler:     keine neue Bewegung ausloesen
/// </summary>
class TrackingController {
public:
  explicit TrackingController(IBallDetector &detector);

  /// Muss im Loop aufgerufen werden, solange der Verfolgungsmodus aktiv ist.
  /// Ruft intern movementController.handleCommand() auf; movementController
  /// .update() muss weiterhin separat im Loop aufgerufen werden.
  void update(MovementController &movementController);

  /// Reine Entscheidungslogik, unabhaengig von Kamera und MovementController
  /// testbar.
  static Button decide(BallPosition position);

private:
  // Drehrichtung fuer die Suche, solange kein Ball sichtbar ist. Platzhalter
  // ohne Kalibrierung/Anforderung aus dem Pflichtenheft.
  static constexpr Button SEARCH_DIRECTION = Button::RotateRight;

  IBallDetector &m_detector;
};
