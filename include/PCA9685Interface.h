#pragma once

#include <chrono>
#include <cstdint>
#include <string>

class PCA9685Interface {
 public:
  PCA9685Interface(
      std::string i2cDevice = "/dev/i2c-1",
      uint8_t address = 0x40,
      uint16_t pwmFrequencyHz = 50,
      uint16_t servoMinUs = 500,
      uint16_t servoMaxUs = 2500);
  ~PCA9685Interface();

  PCA9685Interface(const PCA9685Interface&) = delete;
  PCA9685Interface& operator=(const PCA9685Interface&) = delete;

  bool begin();
  bool isConnected();

  void setSpeed(uint16_t degreesPerSecond);                                     // setzt standard speed
  void setCurrentAngle(uint8_t channel, uint8_t currentAngle);                  // muss vor erstem moveToAngle gesetzt sein 
  void moveToAngle(uint8_t channel, uint8_t targetAngle);                       // smoothe bewegung zum  winkel    
  void moveToAngle(uint8_t channel, uint8_t targetAngle, uint16_t durationMs);  // mit custom geschw.
  void stop(uint8_t channel);                                                   // stoppt Bewegung und haelt aktuellen PWM-Wert
  void stopAll();                                                               // stoppt alle PCA9685-Kanaele
  void update();                                                                // muss im loop aufgerufen werden
  bool isMoving(uint8_t channel) const;                                         // true solange der Kanal interpoliert wird

 private:
  static constexpr uint8_t CHANNEL_COUNT = 16;
  static constexpr uint16_t UNKNOWN_PULSE = 0xFFFF;

  struct ChannelMotion {
    bool active;
    uint16_t startPulseUs;
    uint16_t targetPulseUs;
    std::chrono::steady_clock::time_point startTime;
    uint16_t durationMs;
  };

  void openBus();
  void closeBus();
  void setPwmFrequency(uint16_t frequencyHz);
  void setPwm(uint8_t channel, uint16_t on, uint16_t off);
  void moveToPulse(uint8_t channel, uint16_t targetPulseUs);
  void moveToPulse(uint8_t channel, uint16_t targetPulseUs, uint16_t durationMs);
  void writePulse(uint8_t channel, uint16_t pulseUs);
  uint8_t read8(uint8_t reg);
  void write8(uint8_t reg, uint8_t value);
  uint16_t angleToPulse(uint8_t angle) const;
  uint16_t pulseToTicks(uint16_t pulseUs) const;
  float easeInOut(float progress) const;
  bool isValidChannel(uint8_t channel) const;
  uint16_t durationForPulseDelta(uint16_t pulseDeltaUs) const;

  std::string i2cDevice_;
  uint8_t address_;
  uint16_t pwmFrequencyHz_;
  uint16_t servoMinUs_;
  uint16_t servoMaxUs_;
  uint16_t speedDegreesPerSecond_;
  uint16_t currentPulses_[CHANNEL_COUNT];
  ChannelMotion motions_[CHANNEL_COUNT];
  int fd_;
};
