// V-USB shell for keyboard

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdbool.h>

#include "usbdrv/usbdrv.h"

#include "input.h"
#include "ticks.h"
#include "usb_keyboard.h"

// Default to all ports pulled up, to avoid floating inputs, then let
// modules customize as necessary
static void pullup_ports( void )
{
	DDRB = 0;
	DDRC = 0;
	DDRD = 0;
	
	PORTB = 0xFF;
	PORTC = 0xFF;
	PORTD = 0xFF;
}

static void main_loop( void )
{
	bool force_update = false;
	uint8_t idle_remain = 0;
	for ( ;; )
	{
		if ( ticks_idle() )
		{
			// TODO: OS I use doesn't enable this, so it's untested
			if ( keyboard_idle_period ) // update periodically if USB host wants it
			{
				if ( idle_remain > 4 )
				{
					idle_remain -= 5;
				}
				else
				{
					// flash LED to show that this feature is working
					//DDRC |= 1;
					//PORTC ^= 1;
					
					idle_remain = keyboard_idle_period;
					force_update = true;
				}
			}
		}
		
		input_idle();
		
		// run input_update first so force_update doesn't bypass it
		if ( usb_keyboard_poll() && (input_update() || force_update) )
		{
			force_update = false;
			usb_keyboard_send();
		}
	}
}

int main( void )
{
	pullup_ports();
	
	usb_init();
	while ( !usb_configured() )
		{ }
	
	ticks_init();
	input_init();
	
	main_loop();
	return 0;
}
