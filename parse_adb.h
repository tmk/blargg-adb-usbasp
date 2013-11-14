#ifndef PARSE_ADB_H
#define PARSE_ADB_H

#include <stdbool.h>

// Parse an ADB key press/release byte and update keyboard_modifiers and keyboard_keys.
void parse_adb( unsigned char raw );

// Call before each ADB transaction to process previous press of caps lock.
// True if caps lock was released and usb_keyboard_send() needs to be called.
bool release_caps( void );

// Call when LEDs have changed. Needed for caps lock support.
void leds_changed( unsigned char leds );

#endif
