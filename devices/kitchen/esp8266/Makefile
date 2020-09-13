#git clone https://github.com/plerup/makeEspArduino.git ~/makeEspArduino

CHIP         = esp8266
BOARD        = generic
FLASH_DEF    = 1M128
BUILD_ROOT   = $(PWD)/build
ESP_ADDR		 = 192.168.100.72

# UPLOAD_PORT  = /dev/ttyUSB0

include $(HOME)/makeEspArduino/makeEspArduino.mk

ota_all: ota ota_fs

stack_trace:
	java -jar $(HOME)/makeEspArduino/EspStackTraceDecoder.jar ~/.arduino15/packages/esp8266/tools/xtensa-lx106-elf-gcc/1.20.0-26-gb404fb9-2/bin/xtensa-lx106-elf-addr2line build/esp_generic/esp.elf build/stack
	
monitor:
	socat - /dev/ttyUSB0,b115200,raw,crnl
