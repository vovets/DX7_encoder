#include "encoder.h"

#include <ioavr.h>

#include <stdint.h>
#include <stddef.h>

#define ENC_PIN_A PB3
#define ENC_PIN_B PB4
#define ENC_PIN_A_MASK (1 << ENC_PIN_A)
#define ENC_PIN_B_MASK (1 << ENC_PIN_B)

typedef struct EncoderT {
    EncCallback cwCallback;
    EncCallback ccwCallback;
} Encoder;

static Encoder encoder;

void encInit(void) {
    encoder.cwCallback = NULL;
    encoder.ccwCallback = NULL;

    // activate pull-up
    PORTB |= (1 << ENC_PIN_A);
    PORTB |= (1 << ENC_PIN_B);

    PCMSK_PCINT3 = 1;
    PCMSK_PCINT4 = 1;

    GIMSK_PCIE = 1;
}

void encSetCWCallback(EncCallback c) {
    encoder.cwCallback = c;
}

void encSetCCWCallback(EncCallback c) {
    encoder.ccwCallback = c;
}

#pragma vector=PCINT0_vect
__interrupt void pcint0(void) {
    static int8_t state = 0;
    static uint8_t prevBits = 0;
    static int8_t transitionsTable[16] = {
        0,
        1,
        -1,
        0,

        -1,
        0,
        0,
        1,

        1,
        0,
        0,
        -1,

        0,
        -1,
        1,
        0
    };
    uint8_t newBits = PINB;
    // negation is because active is low
    newBits = ~((((newBits & ENC_PIN_B_MASK) >> ENC_PIN_B) << 1) | ((newBits & ENC_PIN_A_MASK) >> ENC_PIN_A)) & 0x3;
    prevBits = ((prevBits << 2) | newBits) & 0xf;
    state += transitionsTable[prevBits];
    if (state > 3) {
        if (encoder.cwCallback) {
            encoder.cwCallback();
        }
        state = 0;
    } else if (state < -3) {
        if (encoder.ccwCallback) {
            encoder.ccwCallback();
        }
        state = 0;
    }
    if (!prevBits) {
        state = 0;
    }
}
