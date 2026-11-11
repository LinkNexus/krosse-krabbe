#pragma once
#include "BallPosition.h"

/// <summary>
/// Schnittstelle zur Bildverarbeitung (Kamera + OpenCV-Farberkennung). Wird
/// von der Person implementiert, die die Bildverarbeitung uebernimmt; die
/// Verfolgungslogik (TrackingController) kennt nur diese Schnittstelle und
/// keine Kamera-/OpenCV-Details.
/// </summary>
class IBallDetector {
public:
  virtual ~IBallDetector() = default;

  /// Liefert die aktuelle Position des roten Balls im Kamerabild. Muss nicht
  /// blockieren; wird einmal pro Loop-Durchlauf im Verfolgungsmodus
  /// aufgerufen.
  virtual BallPosition detect() = 0;
};
