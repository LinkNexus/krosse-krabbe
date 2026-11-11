#include "PCA9685Interface.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>
#include <utility>

namespace {

constexpr uint8_t MODE1 = 0x00;
constexpr uint8_t MODE2 = 0x01;
constexpr uint8_t PRESCALE = 0xFE;
constexpr uint8_t LED0_ON_L = 0x06;

constexpr uint8_t MODE1_RESTART = 0x80;
constexpr uint8_t MODE1_SLEEP = 0x10;
constexpr uint8_t MODE1_AI = 0x20;
constexpr uint8_t MODE2_OUTDRV = 0x04;

}  // namespace

PCA9685Interface::PCA9685Interface(
    std::string i2cDevice,
    uint8_t address,
    uint16_t pwmFrequencyHz,
    uint16_t servoMinUs,
    uint16_t servoMaxUs)
    : i2cDevice_(std::move(i2cDevice)),
      address_(address),
      pwmFrequencyHz_(pwmFrequencyHz),
      servoMinUs_(servoMinUs),
      servoMaxUs_(servoMaxUs),
      speedDegreesPerSecond_(180),
      currentPulses_{},
      motions_{},
      fd_(-1) {
  for (uint8_t channel = 0; channel < CHANNEL_COUNT; channel++) {
    currentPulses_[channel] = UNKNOWN_PULSE;
    motions_[channel].active = false;
  }
}

PCA9685Interface::~PCA9685Interface() {
  closeBus();
}

bool PCA9685Interface::begin() {
  openBus();

  if (!isConnected()) {
    return false;
  }

  write8(MODE1, MODE1_AI);
  write8(MODE2, MODE2_OUTDRV);
  setPwmFrequency(pwmFrequencyHz_);
  return true;
}

bool PCA9685Interface::isConnected() {
  try {
    read8(MODE1);
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

void PCA9685Interface::setSpeed(uint16_t degreesPerSecond) {
  speedDegreesPerSecond_ = std::max<uint16_t>(1, degreesPerSecond);
}

void PCA9685Interface::setCurrentAngle(uint8_t channel, uint8_t currentAngle) {
  if (!isValidChannel(channel)) {
    throw std::out_of_range("PCA9685-Kanal muss zwischen 0 und 15 liegen");
  }

  motions_[channel].active = false;
  currentPulses_[channel] = angleToPulse(currentAngle);
}

void PCA9685Interface::moveToAngle(uint8_t channel, uint8_t targetAngle) {
  moveToPulse(channel, angleToPulse(targetAngle));
}

void PCA9685Interface::moveToAngle(
    uint8_t channel,
    uint8_t targetAngle,
    uint16_t durationMs) {
  moveToPulse(channel, angleToPulse(targetAngle), durationMs);
}

void PCA9685Interface::stop(uint8_t channel) {
  if (!isValidChannel(channel)) {
    throw std::out_of_range("PCA9685-Kanal muss zwischen 0 und 15 liegen");
  }

  motions_[channel].active = false;

  if (currentPulses_[channel] != UNKNOWN_PULSE) {
    writePulse(channel, currentPulses_[channel]);
  }
}

void PCA9685Interface::stopAll() {
  for (uint8_t channel = 0; channel < CHANNEL_COUNT; channel++) {
    stop(channel);
  }
}

void PCA9685Interface::update() {
  const auto now = std::chrono::steady_clock::now();

  for (uint8_t channel = 0; channel < CHANNEL_COUNT; channel++) {
    if (!motions_[channel].active) {
      continue;
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - motions_[channel].startTime);

    if (elapsed.count() >= motions_[channel].durationMs) {
      motions_[channel].active = false;
      writePulse(channel, motions_[channel].targetPulseUs);
      continue;
    }

    const float progress =
        static_cast<float>(elapsed.count()) / motions_[channel].durationMs;
    const float easedProgress = easeInOut(progress);
    const int32_t pulseDelta =
        static_cast<int32_t>(motions_[channel].targetPulseUs) -
        motions_[channel].startPulseUs;
    const uint16_t pulseUs = static_cast<uint16_t>(
        motions_[channel].startPulseUs + pulseDelta * easedProgress + 0.5f);
    writePulse(channel, pulseUs);
  }
}

bool PCA9685Interface::isMoving(uint8_t channel) const {
  if (!isValidChannel(channel)) {
    throw std::out_of_range("PCA9685-Kanal muss zwischen 0 und 15 liegen");
  }

  return motions_[channel].active;
}

void PCA9685Interface::openBus() {
  if (fd_ >= 0) {
    return;
  }

  fd_ = open(i2cDevice_.c_str(), O_RDWR);
  if (fd_ < 0) {
    throw std::runtime_error("Kann I2C-Device nicht oeffnen: " + i2cDevice_);
  }

  if (ioctl(fd_, I2C_SLAVE, address_) < 0) {
    const std::string error = std::strerror(errno);
    closeBus();
    throw std::runtime_error("Kann PCA9685-Adresse nicht auswaehlen: " + error);
  }
}

void PCA9685Interface::closeBus() {
  if (fd_ >= 0) {
    close(fd_);
    fd_ = -1;
  }
}

void PCA9685Interface::setPwmFrequency(uint16_t frequencyHz) {
  const double prescaleValue = 25000000.0 / (4096.0 * frequencyHz) - 1.0;
  const uint8_t prescale = static_cast<uint8_t>(prescaleValue + 0.5);

  const uint8_t oldMode = read8(MODE1);
  const uint8_t sleepMode = (oldMode & ~MODE1_RESTART) | MODE1_SLEEP;

  write8(MODE1, sleepMode);
  write8(PRESCALE, prescale);
  write8(MODE1, oldMode);
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  write8(MODE1, oldMode | MODE1_RESTART | MODE1_AI);
}

void PCA9685Interface::setPwm(uint8_t channel, uint16_t on, uint16_t off) {
  const uint8_t reg = LED0_ON_L + 4 * channel;
  write8(reg, on & 0xFF);
  write8(reg + 1, on >> 8);
  write8(reg + 2, off & 0xFF);
  write8(reg + 3, off >> 8);
}

void PCA9685Interface::moveToPulse(uint8_t channel, uint16_t targetPulseUs) {
  if (!isValidChannel(channel)) {
    throw std::out_of_range("PCA9685-Kanal muss zwischen 0 und 15 liegen");
  }

  if (currentPulses_[channel] == UNKNOWN_PULSE) {
    writePulse(channel, targetPulseUs);
    return;
  }

  const uint16_t pulseDeltaUs =
      currentPulses_[channel] > targetPulseUs
          ? currentPulses_[channel] - targetPulseUs
          : targetPulseUs - currentPulses_[channel];
  moveToPulse(channel, targetPulseUs, durationForPulseDelta(pulseDeltaUs));
}

void PCA9685Interface::moveToPulse(
    uint8_t channel,
    uint16_t targetPulseUs,
    uint16_t durationMs) {
  if (!isValidChannel(channel)) {
    throw std::out_of_range("PCA9685-Kanal muss zwischen 0 und 15 liegen");
  }

  if (currentPulses_[channel] == UNKNOWN_PULSE || durationMs == 0) {
    motions_[channel].active = false;
    writePulse(channel, targetPulseUs);
    return;
  }

  motions_[channel].active = true;
  motions_[channel].startPulseUs = currentPulses_[channel];
  motions_[channel].targetPulseUs = targetPulseUs;
  motions_[channel].startTime = std::chrono::steady_clock::now();
  motions_[channel].durationMs = durationMs;
}

void PCA9685Interface::writePulse(uint8_t channel, uint16_t pulseUs) {
  setPwm(channel, 0, pulseToTicks(pulseUs));
  currentPulses_[channel] = pulseUs;
}

uint8_t PCA9685Interface::read8(uint8_t reg) {
  if (::write(fd_, &reg, 1) != 1) {
    throw std::runtime_error("I2C-Registeradresse konnte nicht geschrieben werden");
  }

  uint8_t value = 0;
  if (::read(fd_, &value, 1) != 1) {
    throw std::runtime_error("I2C-Register konnte nicht gelesen werden");
  }

  return value;
}

void PCA9685Interface::write8(uint8_t reg, uint8_t value) {
  const uint8_t buffer[2] = {reg, value};
  if (::write(fd_, buffer, sizeof(buffer)) != sizeof(buffer)) {
    throw std::runtime_error("I2C-Register konnte nicht geschrieben werden");
  }
}

uint16_t PCA9685Interface::angleToPulse(uint8_t angle) const {
  angle = std::min<uint8_t>(180, angle);
  return servoMinUs_ +
         (static_cast<uint32_t>(servoMaxUs_ - servoMinUs_) * angle) / 180UL;
}

uint16_t PCA9685Interface::pulseToTicks(uint16_t pulseUs) const {
  const uint32_t periodUs = 1000000UL / pwmFrequencyHz_;
  return static_cast<uint32_t>(pulseUs) * 4096UL / periodUs;
}

float PCA9685Interface::easeInOut(float progress) const {
  progress = std::max(0.0f, std::min(1.0f, progress));
  return progress * progress * (3.0f - 2.0f * progress);
}

bool PCA9685Interface::isValidChannel(uint8_t channel) const {
  return channel < CHANNEL_COUNT;
}

uint16_t PCA9685Interface::durationForPulseDelta(uint16_t pulseDeltaUs) const {
  if (pulseDeltaUs == 0) {
    return 0;
  }

  const uint32_t servoRangeUs = servoMaxUs_ - servoMinUs_;
  const uint32_t degreesTimes1000 = static_cast<uint32_t>(pulseDeltaUs) * 180000UL / servoRangeUs;
  return std::max<uint16_t>(
      1,
      degreesTimes1000 / speedDegreesPerSecond_);
}
