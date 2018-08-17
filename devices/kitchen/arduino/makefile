USER_LIB_PATH = $(shell pwd)/libraries/
# ARDUINO_LIBS  = IRLremote Servo
ARDUINO_DIR   = $(shell echo $$HOME)/Downloads/arduino-1.8.5-linux64/arduino-1.8.5
ARCHITECTURE  = avr
BOARD_TAG     = pro
BOARD_SUB     = 16MHzatmega328

ifneq ($(MAKECMDGOALS), printsize)
# run include with non silent setup
# https://github.com/sudar/Arduino-Makefile
-include /usr/share/arduino/Arduino.mk
endif

# silently compile and print program size
printsize:
	@make size 2>/dev/null | \
	awk '/.*bytes.*/ {A++; if (A <= 2) print $0}'
	@rm -rf build-*

monitor:
	socat - /dev/ttyUSB0,b115200,raw,crnl

test:
	stty -F /dev/ttyUSB0 115200 raw -echo
	cat /dev/ttyUSB0 &
	echo -ne 'digitalWrite 17 1\r\n' > /dev/ttyUSB0
