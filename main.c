#include "pulse_generator.h"
#include "encoder.h"

#include <ioavr.h>
#include <intrinsics.h>
#include <stdint.h>
#include <stdbool.h>

#define CPU_CLOCK 8000000
#define TIMER_PRESCALER 1024

// timer_tick_s = (1 / CPU_CLOCK) * TIMER_PRESCALER =
// = TIMER_PRESCALER / CPU_CLOCK
// timer_ticks = (ms / 1000) / timer_tick_s =
// = ms / (1000 * timer_tick_s) = (ms * CPU_CLOCK) / 1000 * TIMER_PRESCALER
#define MS2TIMER_TICKS(MS) \
    ((MS * (CPU_CLOCK / 1000)) / TIMER_PRESCALER)

const uint16_t onTicks = MS2TIMER_TICKS(250);
const uint16_t offTicks = MS2TIMER_TICKS(250);

static void producePulseA() {
    pgStartChannelA(onTicks, offTicks);
    pgWaitChannelA();
}

static void producePulseB() {
    pgStartChannelB(onTicks, offTicks);
    pgWaitChannelB();
}

#define PULSES_MAX 5
#define PULSES_MIN (-PULSES_MAX)

static volatile int8_t pulsesLeft = 0;

static void cw(void) {
    if (pulsesLeft < PULSES_MAX)
        ++pulsesLeft;
}

static void ccw(void) {
    if (pulsesLeft > PULSES_MIN)
        --pulsesLeft;
}

int main( void )
{
    pgInit();
    encInit();
    encSetCWCallback(&cw);
    encSetCCWCallback(&ccw);
    __enable_interrupt();
    for (;;) {
        if (pulsesLeft > 0) {
            producePulseA();
            __disable_interrupt();
            if (pulsesLeft > 0) {
                --pulsesLeft;
            }
            __enable_interrupt();
        } else if (pulsesLeft < 0) {
            producePulseB();
            __disable_interrupt();
            if (pulsesLeft < 0) {
                ++pulsesLeft;
            }
            __enable_interrupt();
        } else {
            __no_operation();
        }
    }
    return 0;
}
