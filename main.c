// Synchronizes ADB polling with V-USB interrupts

#include "ticks.h"
#include "wake.h"
#include "adb.h"
#include "parse_adb.h"
#include "usb_keyboard.h"
#include "usbdrv/usbdrv.h"

#include "common.h"

// ADB polled no faster than this
//enum { min_adb_ms = 8 }; // (125Hz)  only slightly better for M3501, but hurts others
enum { min_adb_ms = 12 }; // (83Hz) best for M0116

static void update_leds( void )
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

static bool update_idle( void )
{
	if ( ticks_idle() )
	{
		// TODO: get host to actually use this mode so it can be tested
		if ( keyboard_idle_period ) // update periodically if USB host wants it
		{
			enum { units_per_tick = 1000 / 4 / tick_hz };
			
			static byte remain; // *4 = milliseconds
			if ( remain > units_per_tick )
			{
				remain -= units_per_tick;
			}
			else
			{
				remain = keyboard_idle_period;
				return true;
			}
		}
	}
	return false;
}

static void adb_init( void )
{
	adb_host_init();
	
	_delay_ms( 100 );
	
	// Enable separate key codes for left/right shift/control/option keys
	// on Apple Extended Keyboard.
	cli();
	adb_host_listen( 0x2B, 0x02, 0x03 );
	sei();
}

static void init( void )
{
	pullup_ports();

	adb_init();
	
	usb_init();
	while ( !usb_configured() )
		{ }

	ticks_init();
	wake_init();
}

int main( void )
{
	init();
	
	byte prev_time = tclocks();
	for ( ;; )
	{
		wake_idle();
		update_leds();
		
		bool keys_changed = update_idle();
		keys_changed |= release_caps();
		
		static byte extra_key;
		if ( extra_key )
		{
			parse_adb( extra_key );
			extra_key = 0;
			keys_changed = true;
			
			// This would make sense if USB ran faster than every 8ms. Then we'd
			// get this extra update in without affecting ADB poll timing. Currently
			// this would delay the next ADB poll by 8ms.
			//usb_keyboard_send();
			//keys_changed = false;
			
			// And if next ADB event is for this same key, user has exceeded the maximum
			// rate (60Hz) and there's nothing we can do anyway. Sure, if they exceed it
			// for only a few key presses, we could buffer them up and play them back slower,
			// but then later keys would need to be delayed.
		}
		
		// Ensure enough time has passed since last ADB
		// +100 reduces delay so we don't wait too long. It will be rounded back up
		// due to the opportunities occuring only every 4ms.
		enum { min_tclocks = (long) tclock_hz * min_adb_ms / (1000 + 100) };
		for ( ;; )
		{
			// Synchronize with USB interrupt
			usb_keyboard_poll();
			sleep();
			if ( (byte) (tclocks() - prev_time) >= min_tclocks )
				break;
			
			// Use half-interrupts for finer timing granularity
			#ifdef ADB_REDUCED_TIME
				_delay_us( 4000 );
			#else
				_delay_us( 3800 ); // don't cut it so close
			#endif
			if ( (byte) (tclocks() - prev_time) >= min_tclocks )
				break;
		}
		prev_time = tclocks();
		
		// We have at most 4ms here
		// ADB takes 3.26ms when keyboard responds (3.8ms without ADB_REDUCED_TIME)
		cli();
		uint16_t keys = adb_host_kbd_recv();
		sei();
	
		if ( keys != adb_host_nothing && keys != adb_host_error )
		{
			#ifndef NDEBUG
				debug_word( keys );
			#endif
			
			byte key0 = keys >> 8;
			byte key1 = keys & 0xFF;
			
			parse_adb( key0 );
			
			// Ignore duplicate event (some keys do this for whatever reason)
			if ( key0 ^ key1 )
			{
				// key pressed and released in same event, so defer newer event
				if ( (key0 ^ key1) == 0x80 )
					extra_key = key1;
				else if ( (key1 & 0x7F) != 0x7F )
					parse_adb( key1 );
			}
			keys_changed = true;
		}
		
		if ( keys_changed )
		{
			usb_keyboard_send();
			
			#if 0
			debug_byte( keyboard_modifier_keys );
			for ( byte i = 0; i < 6; i++ )
				debug_byte( keyboard_keys [i] );
			debug_newline();
			#endif
		}
	}
}
