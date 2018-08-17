USER_LIB_PATH = $(shell pwd)/libraries/
ARDUINO_LIBS  = SmoothThermistor SerialCommands IRLremote Servo SoftwareSerial
ARDUINO_DIR   = $(shell echo $$HOME)/Downloads/arduino-1.8.5-linux64/arduino-1.8.5
ARCHITECTURE  = avr
BOARD_TAG     = pro
BOARD_SUB     = 16MHzatmega328
# BOARD_TAG    = pro5v328
# ARDUINO_PORT = /dev/ttyACM0
# ARDUINO_PORT = /dev/ttyUSB0

ifeq (, $(filter $(MAKECMDGOALS),printsize monitor))
# run include with non silent setup
# https://github.com/sudar/Arduino-Makefile
-include /usr/share/arduino/Arduino.mk
CXXFLAGS_STD += -DARDUINO=10606 #fix for IRLremote
endif

# silently compile and print program size
printsize:
	@make size 2>/dev/null | \
	awk '/.*bytes.*/ {A++; if (A <= 2) print $0}'
	@rm -rf build-*

monitor:
	socat - /dev/ttyUSB0,b115200,raw,crnl
