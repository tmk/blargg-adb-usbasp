#ifndef TICKS_H
#define TICKS_H

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>

enum { tclock_hz = (F_CPU + 512) / 1024 };
enum { tick_hz = (tclock_hz + 128) / 256 };

extern uint8_t ticks_;

static inline uint8_t ticks( void ) { return ticks_; }

static inline uint8_t tclocks( void ) { return TCNT0; }

static inline void ticks_init( void )
{
	TCCR0 = 5; // 1024 prescaler
}

// True if ticks was incremented
static inline bool ticks_idle( void )
{
	if ( TIFR & (1<<TOV0) )
	{
		TIFR = 1<<TOV0;
		ticks_++;
		return true;
	}
	return false;
}

#endif
