// Interrupt-based timeout that triggers keyboard scanning every second
// for press of power key to wake computer

#include "adb.h"
#include "usbdrv/usbdrv.h"

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

enum { wake_tb_hz = (F_CPU + 512) / 1024 };
enum { wake_period = wake_tb_hz }; // once a second

static void wake_init( void )
{
	TCCR1A = 0;
	TCCR1B = 5<<CS10; // 1024 prescaler
	TIMSK |= 1<<TOIE1;
	TIFR   = 1<<TOV1;
}

static void wake_idle( void )
{
	TCNT1 = ~wake_period;
}

ISR(TIMER1_OVF_vect)
{
	TCNT1 = ~wake_period;
	
	uint16_t keys = adb_host_kbd_recv();
	
	// See if power key pressed
	if ( (keys & 0xFF) == 0x7F || (keys >> 8 & 0xFF) == 0x7F )
	{
		// USB SE0 to wake host
		USBOUT &= ~USBMASK;
		USBDDR |= USBMASK;
		_delay_ms( 10 );
		USBDDR &= ~USBMASK;
		GIFR = 1<<INTF0; // clear since we probably just triggered it
	}
}
