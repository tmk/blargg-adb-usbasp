MCU     = atmega8
F_CPU   = 12000000

DEFINES += -DHAVE_CONFIG_H

INCLUDE += -I.

SOURCES += main.c
SOURCES += ticks.c
SOURCES += input.c
SOURCES += usb_keyboard.c
SOURCES += adb.c
SOURCES += usbdrv/usbdrv.c
SOURCES += usbdrv/usbdrvasm.S
SOURCES += usbdrv/oddebug.c

all:
	@avr-gcc -std=gnu99 -W -Wall \
		$(DEFINES) -mmcu=$(MCU) -DF_CPU=$(F_CPU)UL \
		-Wno-unused-function -Wall -Wstrict-prototypes -Werror=implicit-function-declaration \
		-Os \
		-Dinline='inline __attribute__((always_inline))' \
		-funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums \
		-Wl,--relax -o main.elf \
		$(INCLUDE) $(SOURCES)
	@avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature main.elf main.hex

clean:
	rm main.hex
	rm main.elf

flash: all
	avrdude -v -p m8 -c usbasp -e -U flash:w:main.hex

.PHONY: all clean flash
