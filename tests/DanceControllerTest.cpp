#include "DanceController.h"
#include "CommandArbiter.h"
#include "WebServer.h"

#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct FakeMotorState {
  std::array<float, 4> upperDefaults = {80.0f, 82.0f, 100.0f, 102.0f};
  std::array<float, 4> lowerDefaults = {88.0f, 89.0f, 91.0f, 92.0f};
  std::vector<MotorCommand> commands;
  bool moving = false;
  bool acceptCommands = true;
};

FakeMotorState fake;
int failures = 0;

void expect(bool condition, const std::string &message) {
  if (condition) {
    return;
  }

  ++failures;
  std::cerr << "FEHLER: " << message << std::endl;
}

bool almostEqual(float left, float right) {
  return std::fabs(left - right) < 0.001f;
}

const MotorCommand &commandAt(std::size_t pose, LegId leg, LegType type) {
  const std::size_t typeOffset = type == UpperLeg ? 0 : 1;
  return fake.commands.at(pose * 8 + static_cast<std::size_t>(leg) * 2 +
                          typeOffset);
}

void expectStandPose(std::size_t pose) {
  for (uint8_t i = 0; i < 4; ++i) {
    const LegId leg = static_cast<LegId>(i);
    expect(almostEqual(commandAt(pose, leg, UpperLeg).angleToMove,
                       fake.upperDefaults[i]),
           "Standpose nutzt nicht den individuellen Oberschenkel-Default");
    expect(almostEqual(commandAt(pose, leg, LowerLeg).angleToMove,
                       20.0f),
           "Standpose verwendet nicht den Unterschenkel-Standwinkel");
  }
}

void finishCurrentPose(DanceController &dance,
                       DanceController::TimePoint &now) {
  dance.update(now); // Moving -> Holding
  now += std::chrono::seconds(2);
  dance.update(now); // Holding -> naechste Pose bzw. Idle
}

void testCompleteSingleShotDance() {
  MotorManager motors;
  DanceController dance(motors);
  DanceController::TimePoint now{};

  expect(dance.start(), "Erster Tanzstart wurde abgelehnt");
  expect(!dance.start(), "Ein aktiver Tanz konnte ein zweites Mal starten");
  expect(dance.isActive(), "Tanz ist direkt nach start() nicht aktiv");
  expect(fake.commands.size() == 8,
         "Startpose erzeugt nicht genau acht Motorbefehle");
  expectStandPose(0);

  // Auch eine weit ueberschrittene Zeit darf eine laufende Servo-Bewegung
  // nicht ueberspringen.
  fake.moving = true;
  dance.update(now + std::chrono::seconds(30));
  expect(fake.commands.size() == 8,
         "Neue Pose wurde vor Ende der Servobewegung gestartet");
  fake.moving = false;

  // Dreizehn Posen insgesamt: Startstand, Tanz mit zwei Links-Rechts-
  // Wiederholungen und Abschlussstand.
  for (std::size_t pose = 0; pose < 13; ++pose) {
    finishCurrentPose(dance, now);
  }

  expect(!dance.isActive(), "Tanz endet nicht nach der Abschluss-Pose");
  expect(!dance.hasFault(), "Fehlerfreier Tanz meldet einen Fault");
  expect(fake.commands.size() == 13 * 8,
         "Tanz erzeugt nicht fuer jede Pose acht Motorbefehle");
  expectStandPose(12);

  // Hocke und Strecken nutzen absolute, direkt kalibrierbare Winkel.
  for (uint8_t i = 0; i < 4; ++i) {
    const LegId leg = static_cast<LegId>(i);
    expect(almostEqual(commandAt(1, leg, UpperLeg).angleToMove,
                       fake.upperDefaults[i]),
           "Hocke veraendert unerwartet einen Oberschenkel");
    expect(almostEqual(commandAt(1, leg, LowerLeg).angleToMove, 30.0f),
           "Hocke verwendet nicht den Unterschenkel-Hockwinkel");
    expect(almostEqual(commandAt(3, leg, LowerLeg).angleToMove, 10.0f),
           "Strecken verwendet nicht den Unterschenkel-Streckwinkel");
  }

  // Seitliches Wiegen: Oberschenkel bleiben auf Default. Die Unterschenkel
  // werden links/rechts gegensinnig bewegt und tauschen danach die Richtung.
  for (uint8_t i = 0; i < 4; ++i) {
    const LegId leg = static_cast<LegId>(i);
    const bool left = leg == FrontLeft || leg == BackLeft;

    expect(almostEqual(commandAt(5, leg, UpperLeg).angleToMove,
                       fake.upperDefaults[i]),
           "Seitliches Wiegen veraendert unerwartet einen Oberschenkel");
    expect(almostEqual(commandAt(5, leg, LowerLeg).angleToMove,
                       left ? 35.0f : 5.0f),
           "Erste Wiegepose bewegt die Seiten nicht gegensinnig");
    expect(almostEqual(commandAt(7, leg, LowerLeg).angleToMove,
                       left ? 5.0f : 35.0f),
           "Zweite Wiegepose kehrt die Seiten nicht um");
    expect(almostEqual(commandAt(9, leg, LowerLeg).angleToMove,
                       left ? 35.0f : 5.0f),
           "Zweiter Wiegedurchlauf wiederholt die erste Seite nicht");
    expect(almostEqual(commandAt(11, leg, LowerLeg).angleToMove,
                       left ? 5.0f : 35.0f),
           "Zweiter Wiegedurchlauf wiederholt die Gegenseite nicht");
  }

  const std::size_t commandCountAfterFinish = fake.commands.size();
  dance.update(now + std::chrono::seconds(10));
  expect(fake.commands.size() == commandCountAfterFinish,
         "Inaktiver Tanz erzeugt weitere Befehle");
}

void testRejectedPoseLocksController() {
  MotorManager motors;
  fake.acceptCommands = false;
  DanceController dance(motors);

  std::ostringstream expectedError;
  std::streambuf *previousErrorBuffer = std::cerr.rdbuf(expectedError.rdbuf());
  const bool started = dance.start();
  std::cerr.rdbuf(previousErrorBuffer);

  expect(!started, "Abgelehnte Startpose meldet trotzdem Erfolg");
  expect(dance.isActive(), "Fault gibt andere Bewegungsbefehle wieder frei");
  expect(dance.hasFault(), "Abgelehnte Pose setzt keinen Fault-Zustand");
  expect(fake.commands.empty(), "Abgelehnte Pose wurde teilweise ausgefuehrt");
}

void testDanceButtonIsOneShotEvent() {
  WebServer webServer;
  const WebServer::TimePoint clickTime{};

  WebServer::InputSnapshot input = webServer.pollInput(clickTime);
  expect(!input.danceRequested, "Webserver meldet ohne Klick einen Tanzwunsch");
  expect(input.command == Button::None,
         "Webserver startet nicht ohne Eingabe im Idle-Zustand");

  webServer.setPressedButton(Button::DanceMode, clickTime);
  input = webServer.pollInput(clickTime + std::chrono::milliseconds(201));
  expect(input.command == Button::None,
         "Abgelaufener Button-Level bleibt laenger als 200 ms aktiv");
  expect(input.danceRequested,
         "Tanz-Klick geht zusammen mit dem 200-ms-Button-Level verloren");

  input = webServer.pollInput(clickTime + std::chrono::milliseconds(201));
  expect(!input.danceRequested,
         "Ein Tanz-Klick wurde mehr als einmal ausgeliefert");

  webServer.setActiveMode(Button::DanceMode);
  expect(webServer.getActiveMode() == Button::DanceMode,
         "Aktiver Tanzstatus wurde nicht gespeichert");
}

void testCommandsAreDiscardedUntilReleaseAfterDance() {
  CommandArbiter arbiter;
  using Action = CommandArbiter::Action;

  expect(arbiter.decide(false, Button::None, true) == Action::StartDance,
         "Tanz-Klick startet keinen Tanz");
  expect(!arbiter.commandsArmed(),
         "Andere Befehle bleiben nach Tanzstart freigeschaltet");

  expect(arbiter.decide(true, Button::Forward, false) == Action::ContinueDance,
         "Fahrbefehl unterbricht den laufenden Tanz");
  expect(arbiter.decide(true, Button::DanceMode, true) == Action::ContinueDance,
         "Weiterer Tanz-Klick wird waehrend des Tanzes vorgemerkt");

  // Ein waehrend des Endes gehaltener Befehl bleibt gesperrt.
  expect(arbiter.decide(false, Button::Forward, false) ==
             Action::WaitForButtonRelease,
         "Gehaltener Fahrbefehl wird nach Tanzende nachgeholt");
  expect(arbiter.decide(false, Button::DanceMode, true) ==
             Action::WaitForButtonRelease,
         "Gehaltener Tanzbutton startet direkt einen zweiten Tanz");

  // None schaltet nur frei; erst der naechste Sample wird ausgefuehrt.
  expect(arbiter.decide(false, Button::None, false) ==
             Action::WaitForButtonRelease,
         "Loslassen fuehrt unerwartet einen Befehl aus");
  expect(arbiter.commandsArmed(), "Loslassen schaltet Befehle nicht frei");
  expect(arbiter.decide(false, Button::Forward, false) ==
             Action::HandleMovementCommand,
         "Neuer Fahrbefehl nach dem Loslassen bleibt gesperrt");
}

void testPcaReportsScheduledMotion() {
  PCA9685Interface driver;

  expect(!driver.isMoving(0),
         "Unbenutzter PCA-Kanal meldet eine laufende Bewegung");
  driver.setSpeed(20);
  driver.setCurrentAngle(0, 90);
  driver.moveToAngle(0, 100);
  expect(driver.isMoving(0),
         "Geplante PCA-Interpolation wird nicht als Bewegung gemeldet");
}

} // namespace

// Schlanker MotorManager-Fake: DanceController wird gegen dieselbe API wie in
// Produktion gelinkt, ohne PCA9685 oder /dev/i2c-1 zu initialisieren.
MotorManager::MotorManager() { fake = FakeMotorState{}; }

float MotorManager::getDefaultAngle(LegId leg, LegType type) {
  const std::size_t index = static_cast<std::size_t>(leg);
  return type == UpperLeg ? fake.upperDefaults[index]
                          : fake.lowerDefaults[index];
}

bool MotorManager::canExecute(const MotorCommand &) const {
  return fake.acceptCommands;
}

bool MotorManager::execute(MotorCommand command) {
  if (!fake.acceptCommands) {
    return false;
  }

  fake.commands.push_back(command);
  return true;
}

bool MotorManager::isMoving() const { return fake.moving; }

int main() {
  testCompleteSingleShotDance();
  testRejectedPoseLocksController();
  testDanceButtonIsOneShotEvent();
  testCommandsAreDiscardedUntilReleaseAfterDance();
  testPcaReportsScheduledMotion();

  if (failures != 0) {
    std::cerr << failures << " Tanztest(s) fehlgeschlagen." << std::endl;
    return 1;
  }

  std::cout << "Alle DanceController-Tests bestanden." << std::endl;
  return 0;
}
