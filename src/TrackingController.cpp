#include "TrackingController.h"

TrackingController::TrackingController(IBallDetector &detector)
    : m_detector(detector) {}

Button TrackingController::decide(BallPosition position) {
  switch (position) {
  case BallPosition::Center:
    return Button::Forward;
  case BallPosition::Left:
    return Button::RotateLeft;
  case BallPosition::Right:
    return Button::RotateRight;
  case BallPosition::NotVisible:
    return SEARCH_DIRECTION;
  case BallPosition::ErrorCam:
  case BallPosition::ErrorNoDef:
  default:
    return Button::None;
  }
}

void TrackingController::update(MovementController &movementController) {
  BallPosition position = m_detector.detect();
  movementController.handleCommand(decide(position));
}
