CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 -Iinclude
LDLIBS ?= $(shell pkg-config --libs cpp-httplib 2>/dev/null) -pthread -ljpeg
TARGET = crabbyProgram

# Standardmaessig ECHTE Pi-Kamera (Ernstfall braucht keine Extra-Eingabe).
# Fuer Entwicklung auf dem PC ohne Kamera-Hardware: make USE_PI_CAMERA=0
USE_PI_CAMERA ?= 1

SRC = src/main.cpp src/PCA9685Interface.cpp src/MotorManager.cpp \
      src/MovementController.cpp src/DanceController.cpp \
      src/ControllerInput.cpp \
      src/Webserver/WebServer.cpp src/Camera/image_analysis.cpp \
      src/Camera/jpeg_codec.cpp src/Camera/CameraInterface.cpp \
      src/TrackingController.cpp

ifeq ($(USE_PI_CAMERA),1)
    SRC += src/Camera/capture_pi.cpp
else
    SRC += src/Camera/capture_mock.cpp
endif

HEADERS = $(wildcard include/*.h)
TEST_TARGET = danceControllerTests
TEST_SRC = tests/DanceControllerTest.cpp src/DanceController.cpp \
           src/PCA9685Interface.cpp src/Webserver/WebServer.cpp

.PHONY: all clean test gen-test-image

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDLIBS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(TEST_SRC) -o $(TEST_TARGET) $(LDLIBS)

# Erzeugt test_images/sample_red.jpg fuer den Mock-Modus.
# Separates kleines Binary, braucht kein cpp-httplib/pthread. "make gen-test-image"
gen-test-image: src/Camera/gen_test_image.cpp src/Camera/jpeg_codec.cpp
	$(CXX) $(CXXFLAGS) src/Camera/gen_test_image.cpp src/Camera/jpeg_codec.cpp -o gen_test_image -ljpeg

clean:
	rm -f $(TARGET) $(TEST_TARGET) gen_test_image
