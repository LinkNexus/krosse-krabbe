#pragma once
#include "IBallDetector.h"

/// <summary>
/// Platzhalter-Implementierung von IBallDetector, solange die echte
/// Bildverarbeitung (Kamera + OpenCV) noch nicht eingebunden ist. Meldet
/// immer "kein Ball sichtbar", damit der Verfolgungsmodus in die Suchdrehung
/// geht, ohne abzustuerzen. Muss durch die echte Bildverarbeitung ersetzt
/// werden.
/// </summary>
class StubBallDetector : public IBallDetector {
public:
  BallPosition detect() override { return BallPosition::NotVisible; }
};
