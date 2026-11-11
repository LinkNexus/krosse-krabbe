#ifdef __linux__
#include <fcntl.h>
#include <linux/joystick.h>
#include <unistd.h>
#endif

#include "Button.h"
#include "CameraInterface.h"
#include "CommandArbiter.h"
#include "ControllerInput.h"
#include "DanceController.h"
#include "MovementController.h"
#include "TrackingController.h"
#include "WebServer.h" // Dafuer muss cpp-httplib eingebunden sein

#include <chrono>
#include <csignal>
#include <cstdint>
#include <exception>
#include <iostream>
#include <thread>

namespace {
constexpr uint16_t LOOP_DELAY_MS = 10;

volatile std::sig_atomic_t keepRunning = 1;

void handleSignal(int) { keepRunning = 0; }

} // namespace

int main() {
  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);

#ifdef __linux__
  const int controller = open("/dev/input/js0", O_RDONLY | O_NONBLOCK);
  if (controller < 0) {
    std::cout << "Controller konnte nicht geoeffnet werden; "
                 "Websteuerung bleibt aktiv."
              << std::endl;
  }
#endif

  try {
    MovementController movementController;
    DanceController danceController(movementController.motorManager());
    WebServer webServer;
    webServer.initializeServer();

    CameraBallDetector ballDetector;
    TrackingController trackingController(ballDetector);
    bool trackingModeActive = false;
    Button previousCommand = Button::None;
    Button controllerCommand = Button::None;

    if (!movementController.begin()) {
      return 1;
    }

    std::cout << "Roboter steht in der Startposition." << std::endl;
    std::cout << "Zum Stoppen Ctrl+C druecken." << std::endl;

    CommandArbiter commandArbiter;
    bool idleDefaultRequested = true; // begin() hat Defaults bereits angefahren
    auto nextLoop = std::chrono::steady_clock::now();

    while (keepRunning) { // main loop
      nextLoop += std::chrono::milliseconds(LOOP_DELAY_MS);

#ifdef __linux__
      if (controller >= 0) {
        js_event event{};
        while (read(controller, &event, sizeof(event)) == sizeof(event)) {
          if ((event.type & JS_EVENT_BUTTON) != 0 &&
              ControllerInput::isMappedButton(event.number)) {
            controllerCommand = ControllerInput::update(event);
          }
        }
      }
#endif

      const WebServer::InputSnapshot input = webServer.pollInput();
      const bool webInputActive =
          input.command != Button::None || input.danceRequested;
      const Button command =
          webInputActive ? input.command : controllerCommand;
      const bool danceRequested =
          input.danceRequested || command == Button::DanceMode;
      bool danceOwnsMotors = false;

      switch (commandArbiter.decide(danceController.isActive(), command,
                                    danceRequested)) {
      case CommandArbiter::Action::ContinueDance:
        // Waehrend des Tanzes werden alle anderen Eingaben verworfen.
        danceController.update();
        danceOwnsMotors = true;
        webServer.setActiveMode(Button::DanceMode);
        break;

      case CommandArbiter::Action::WaitForButtonRelease:
        movementController.handleCommand(Button::None);
        webServer.setActiveMode(Button::None);
        break;

      case CommandArbiter::Action::StartDance:
        // Tanz und Tracking verwenden denselben MovementController und sind
        // deshalb gegenseitig exklusiv. Ein Tanzstart beendet Tracking.
        if (trackingModeActive) {
          trackingModeActive = false;
          ballDetector.setActive(false);
        }
        movementController.reset();

        if (danceController.start()) {
          danceOwnsMotors = true;
          webServer.setActiveMode(Button::DanceMode);
        } else {
          danceOwnsMotors = danceController.isActive();
          std::cerr << "Fehler: Tanz konnte nicht gestartet werden."
                    << std::endl;
          webServer.setActiveMode(danceOwnsMotors ? Button::DanceMode
                                                  : Button::None);
        }
        break;

      case CommandArbiter::Action::HandleMovementCommand:
        // TrackingMode wird per Tastendruck umgeschaltet. Nur auf die Flanke
        // reagieren, da der Webserver den Button bis zu 200 ms wiederholt.
        if (command == Button::TrackingMode &&
            previousCommand != Button::TrackingMode) {
          trackingModeActive = !trackingModeActive;
          ballDetector.setActive(trackingModeActive);
          movementController.reset();
        }

        if (trackingModeActive) {
          trackingController.update(movementController);
          webServer.setActiveMode(Button::TrackingMode);
        } else {
          const Button movementCommand =
              command == Button::TrackingMode ? Button::None : command;
          movementController.handleCommand(movementCommand);
          webServer.setActiveMode(movementCommand);
        }
        break;
      }

      // Auch waehrend des Tanzes aktualisieren, damit Klicks in dieser Zeit
      // weder nachgeholt noch als spaetere Tracking-Flanke interpretiert werden.
      previousCommand = command;

      // Beim Eintritt in Idle genau einmal langsam zu den Motor-Defaults
      // fahren. Insbesondere nach dem Tanz senkt sich die Krabbe dadurch
      // kontrolliert vom Stand auf den Bauch ab. Wiederholtes Kommandieren im
      // 10-ms-Loop wuerde die laufende Servo-Interpolation immer neu ansetzen.
      if (webServer.getActiveMode() == Button::None) {
        if (!idleDefaultRequested) {
          movementController.motorManager().goToDefaultPosition();
          idleDefaultRequested = true;
        }
      } else {
        idleDefaultRequested = false;
      }

      if (danceOwnsMotors) {
        // Keine andere Controller-Zustandsmaschine darf Tanzposen ersetzen.
        movementController.motorManager().update();
      } else {
        movementController.update();
      }

      std::this_thread::sleep_until(nextLoop);
    }

    ballDetector.setActive(false);
    std::cout << "Programm beendet." << std::endl;
  } catch (const std::exception &error) {
    std::cerr << "Fehler: " << error.what() << std::endl;
    return 1;
  }

  return 0;
}
