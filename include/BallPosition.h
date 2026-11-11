#pragma once

/// <summary>
/// Ergebnis der Bildverarbeitung (Kamera + Farberkennung) fuer den
/// Verfolgungsmodus. Wird von der Ballverfolgung/Bildverarbeitung (separates
/// Teammitglied) pro Frame ermittelt: es wird nur die horizontale Position
/// des groessten roten Objekts im Bild gebraucht (siehe Technische
/// Spezifikation, Funktion 4, T4/T5).
/// </summary>
enum class BallPosition {
  NotVisible, // kein rotes Objekt im aktuellen Frame gefunden
  Left,
  Center,
  Right,
  ErrorCam,
  ErrorNoDef
};
