MCU      = atmega8
F_CPU    = 12000000

DEFINES += -DHAVE_CONFIG_H
DEFINES += -DNDEBUG

INCLUDE += -I.

SOURCES += main.c
SOURCES += ticks.c
SOURCES += parse_adb.c
SOURCES += adb.c
SOURCES += usb_keyboard.c
SOURCES += usbdrv/usbdrv.c
SOURCES += usbdrv/usbdrvasm.S

all:
	avr-gcc -std=gnu99 -W -Wall \
		-Wno-unused-function -Wstrict-prototypes \
		-Werror=implicit-function-declaration \
		$(DEFINES) -mmcu=$(MCU) -DF_CPU=$(F_CPU)UL \
		-Os -Dinline='inline __attribute__((always_inline))' \
		-funsigned-char -funsigned-bitfields \
		-fpack-struct -fshort-enums \
		-ffunction-sections -fdata-sections -Wl,--relax,--gc-sections \
		-o main.elf $(INCLUDE) $(SOURCES)
	avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature main.elf main.hex

clean:
	-rm main.hex
	-rm main.elf

flash: all
	avrdude -v -p m8 -c usbasp -e -U flash:w:main.hex

.PHONY: all clean flash
