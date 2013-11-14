// Sends ADB key presses to usb_keyboard and handles locking caps lock

#include "input.h"

#include "adb.h"
#include "ticks.h"
#include "keymap.h"
#include "usb_keyboard.h"
#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "config.h"

enum { min_period = 7 }; // ms

// disable for now
#define debug_str	1 ? (void) 0 : (void) 

typedef unsigned char byte;

enum { max_keys = 6 };
enum { released_mask = 0x80 };

static void handle_key( byte raw )
{
	byte code = usb_from_adb_code( raw & 0x7F );
	if ( !code )
		return;
	
	if ( KC_LCTRL <= code && code <= KC_RGUI )
	{
		// Modifier; route to mask rather than keys list
		byte mask = 1 << (code - KC_LCTRL);
		keyboard_modifier_keys &= ~mask;
		if ( !(raw & released_mask) )
			keyboard_modifier_keys |= mask;
	}
	else
	{
		// Find code in list
		byte i = 0;
		do
		{
			if ( keyboard_keys [i] == code )
				break;
			i++;
		}
		while ( i < max_keys );
		
		if ( raw & released_mask )
		{
			// Released
			if ( i >= max_keys )
			{
				debug_str( "released key not in list\n" );
			}
			else
			{
				// Remove from list
				for ( ; i < max_keys - 1; i++ )
					keyboard_keys [i] = keyboard_keys [i + 1];
				
				keyboard_keys [i] = 0;
			}
		}
		else
		{
			// Pressed
			if ( i < max_keys )
			{
				debug_str( "pressed key already in list\n" );
			}
			else
			{
				// Add to list
				i = 0;
				do
				{
					if ( keyboard_keys [i] == 0 )
					{
						keyboard_keys [i] = code;
						break;
					}
					i++;
				}
				while ( i < max_keys );
			
				if ( i >= max_keys )
					debug_str( "too many keys pressed\n" );
			}
		}
	}
}

#ifdef UNLOCKED_CAPS

static void handle_key_caps( byte raw )
{
	handle_key( raw );
}

static bool release_caps( void ) { return false; }

static void leds_changed( byte leds ) { }

#else

enum { caps_mask = 2 };

static byte caps_release_delay;
static byte caps_on;
static byte prev_caps_led;

static void press_caps_momentarily( void )
{
	handle_key( ADB_CAPS );
	caps_release_delay = 1;
}

static bool release_caps( void )
{
	if ( caps_release_delay && !--caps_release_delay )
	{
		handle_key( ADB_CAPS | released_mask );
		return true;
	}
	return false;
}

static void leds_changed( byte leds )
{
	// only update our flag when USB host changes caps LED
	byte caps_led = leds & caps_mask;
	if ( prev_caps_led != caps_led )
	{
		prev_caps_led = caps_led;
		caps_on = caps_led;
	}
}

static void handle_key_caps( byte raw )
{
	if ( (raw & 0x7F) != ADB_CAPS )
	{
		handle_key( raw );
		return;
	}
	
	byte new_caps = (raw & released_mask) ? 0 : caps_mask;
	if ( caps_on != new_caps )
	{
		// in case host doesn't handle LED, keep ours updated
		caps_on = new_caps;
		press_caps_momentarily();
	}
}

#endif

void input_init( void )
{
	adb_host_init();
	
	// Enable separate key codes for left/right shift/control/option keys
	// on Apple Extended Keyboard.
	
    adb_host_listen( 0x2B, 0x02, 0x03 );
}

void input_idle( void )
{
	static byte leds = -1;
	byte new_leds = keyboard_leds;
	if ( leds != new_leds )
	{
		leds = new_leds;
		leds_changed( new_leds );
		adb_host_kbd_led( ~new_leds & 0x07 );
	}
}

static inline void sleep( void )
{
	#ifdef SMCR
		SMCR &= ~(1<<SM2 | 1<<SM1 | 1<<SM0);
	#endif
	sleep_enable();
	sleep_cpu();
	sleep_disable();
}

bool input_update( void )
{
	bool changed = release_caps();
	
	for ( ;; )
	{
		// Sync with USB interrupt
		sleep();
		
		// Ensure that it's not too soon to poll ADB
		static byte prev_time;
		byte time = tclocks();
		byte diff = time - prev_time;
		enum { min_tclocks = (long) tclocks_per_sec * min_period / 1000 };
		if ( diff > (byte) min_tclocks )
		{
			prev_time = time;
			break;
		}
	}
	
	// Now we can disable interrupts without interfering with USB
	cli();
	uint16_t keys = adb_host_kbd_recv();
	sei();
	
	if ( keys == adb_host_nothing )
		return changed;
		
	if ( keys == adb_host_error )
		return changed;
	
	// Split the two key events
	handle_key_caps( keys >> 8 );
	byte key = keys & 0xFF;
	if ( (key & 0x7F) != 0x7F )
		handle_key_caps( key );
	
	return true;
}
