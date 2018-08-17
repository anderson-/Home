USER_LIB_PATH = $(shell pwd)/libraries/
ARDUINO_LIBS  = EEPROM Wire Time
ARDUINO_DIR   = $(shell echo $$HOME)/Downloads/arduino-1.8.5-linux64/arduino-1.8.5
ARCHITECTURE  = avr
BOARD_TAG     = pro
BOARD_SUB     = 16MHzatmega328
BAUDRATE      = 9600

ifeq (, $(filter $(MAKECMDGOALS),printsize monitor clock-sync test))
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
	socat - /dev/ttyUSB0,b${BAUDRATE},raw,crnl

clock-sync:
	echo "T$$(echo $$(date +%s)-3*60*60 | bc)"
	socat - /dev/ttyUSB0,b${BAUDRATE},raw,crnl

test:
	stty -F /dev/ttyUSB0 ${BAUDRATE} raw -echo
	cat /dev/ttyUSB0 &
	echo -ne 'digitalWrite 17 1\r\n' > /dev/ttyUSB0
