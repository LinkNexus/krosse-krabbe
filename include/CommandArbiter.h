#pragma once

#include "Button.h"

#include <cstdint>

/// Entscheidet, welcher Controller in einem Mainloop-Durchlauf die Motoren
/// verwenden darf. Befehle waehrend eines Tanzes werden nicht gespeichert.
class CommandArbiter {
public:
  enum class Action : uint8_t {
    ContinueDance,
    StartDance,
    WaitForButtonRelease,
    HandleMovementCommand,
  };

  Action decide(bool danceActive, Button command, bool danceRequested) {
    if (danceActive) {
      m_commandsArmed = false;
      return Action::ContinueDance;
    }

    if (!m_commandsArmed) {
      // Auch der erste None-Sample wird nur zum erneuten Scharfschalten
      // verwendet. Erst ein danach empfangener Befehl darf ausgefuehrt werden.
      if (command == Button::None) {
        m_commandsArmed = true;
      }
      return Action::WaitForButtonRelease;
    }

    if (danceRequested || command == Button::DanceMode) {
      m_commandsArmed = false;
      return Action::StartDance;
    }

    return Action::HandleMovementCommand;
  }

  bool commandsArmed() const { return m_commandsArmed; }

private:
  bool m_commandsArmed = true;
};
