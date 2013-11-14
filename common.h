#ifndef COMMON_H
#define COMMON_H

#include "config.h"

#include <stdbool.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

typedef unsigned char byte;

#define debug_str( a )		((void) 0)
#define debug_byte( a )		((void) 0)
#define debug_word( a )		((void) 0)
#define debug_newline( a )	((void) 0)

static void sleep( void )
{
	#ifdef SMCR
		SMCR &= ~(1<<SM2 | 1<<SM1 | 1<<SM0);
	#endif
	sleep_enable();
	sleep_cpu();
	sleep_disable();
}

// Default to all ports pulled up, to avoid floating inputs, then let
// modules customize as necessary
static void pullup_ports( void )
{
	#ifdef DDRA
		DDRA  = 0;
		PORTA = 0xFF;
	#endif
	
	DDRB  = 0;
	DDRC  = 0;
	DDRD  = 0;
	PORTB = 0xFF;
	PORTC = 0xFF;
	PORTD = 0xFF;
	
	#ifdef DDRE
		DDRE  = 0;
		PORTE = 0xFF;
	#endif
	
	#ifdef DDRF
		DDRF  = 0;
		PORTF = 0xFF;
	#endif
}

#endif
