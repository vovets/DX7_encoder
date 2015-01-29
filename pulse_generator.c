#include <ioavr.h>
#include <intrinsics.h>

#include <stdint.h>
#include <stdbool.h>

#define CHANNEL_A_PIN PB0
#define CHANNEL_B_PIN PB1

typedef union {
        uint16_t ticks;
        struct {
            uint8_t ticksL;
            uint8_t ticksH;
        };
} Ticks;

struct ChannelT;

typedef void (*CompareMatchIntHandler)(struct ChannelT*);
typedef void (*SetOut)(void);
typedef void (*SetCompareMatchInt)(bool);
typedef void (*SetOCRegister)(uint8_t);

typedef struct ChannelT {
    Ticks onTicks;
    Ticks offTicks;
    CompareMatchIntHandler handler;
    SetCompareMatchInt setCompareMatchInt;
    SetOCRegister setOCRegister;
    SetOut setOutActiveOnInt;
    SetOut setOutInactiveOnInt;
    unsigned char volatile __io* oCRegister;
    bool finished;
} Channel;

static Channel channelA;
static Channel channelB;

static void handler_HighTimeStart(Channel*);
static void handler_LowTimeStart(Channel*);
static void handler_LowTimeContinue(Channel*);
static void handler_LowTimeFinish(Channel*);

static void setOutHighOnIntA(void);
static void setOutHighOnIntB(void);
static void setOutLowOnIntA(void);
static void setOutLowOnIntB(void);

static void setOCRegisterA(uint8_t value);
static void setOCRegisterB(uint8_t value);

static void initChannel(Channel*, uint16_t onTicks, uint16_t offTicks);
static void waitChannel(Channel*);

static void setCompareMatchIntA(bool enable);
static void setCompareMatchIntB(bool enable);

static void startTimer(void);
static void stopTimer(void);

void pgInit(void) {
    channelA.setOutInactiveOnInt = &setOutHighOnIntA;
    channelA.setOutActiveOnInt = &setOutLowOnIntA;
    channelA.setCompareMatchInt = &setCompareMatchIntA;
    channelA.setOCRegister = &setOCRegisterA;

    channelB.setOutInactiveOnInt = &setOutHighOnIntB;
    channelB.setOutActiveOnInt = &setOutLowOnIntB;
    channelB.setCompareMatchInt = &setCompareMatchIntB;
    channelB.setOCRegister = &setOCRegisterB;

    DDRB |= (1 << CHANNEL_A_PIN);
    DDRB |= (1 << CHANNEL_B_PIN);

    channelA.setOutInactiveOnInt();
    TCCR0B_FOC0A = 1;

    channelB.setOutInactiveOnInt();
    TCCR0B_FOC0B = 1;
}

void pgStartChannelA(uint16_t onTicks, uint16_t offTicks) {
    initChannel(&channelA, onTicks, offTicks);
    startTimer();
}

void pgStartChannelB(uint16_t onTicks, uint16_t offTicks) {
    initChannel(&channelB, onTicks, offTicks);
    startTimer();
}

void pgWaitChannelA(void) {
    waitChannel(&channelA);
}

void pgWaitChannelB(void) {
    waitChannel(&channelB);
}

static void initChannel(Channel* channel, uint16_t onTicks, uint16_t offTicks) {
    channel->handler = &handler_HighTimeStart;
    channel->onTicks.ticks = onTicks;
    channel->offTicks.ticks = offTicks;
    channel->finished = false;
    channel->setOCRegister(0);
    channel->setOutActiveOnInt();
    channel->setCompareMatchInt(true);
}

static void waitChannel(Channel* channel) {
    while (!channel->finished) {
        __no_operation();
    }
}

static void handler_HighTimeStart(Channel* channel) {
    if (channel->onTicks.ticks <= 256) {
        channel->setOutInactiveOnInt();
        channel->setOCRegister(channel->onTicks.ticksL);
        channel->handler = &handler_LowTimeStart;
        return;
    }
    --channel->onTicks.ticksH;
}

static void handler_LowTimeStart(Channel* channel) {
    uint8_t remainder = 256 - channel->onTicks.ticksL;
    if (channel->offTicks.ticks <= remainder) {
        channel->setOCRegister(channel->onTicks.ticksL + channel->offTicks.ticksL);
        channel->handler = &handler_LowTimeFinish;
        return;
    }
    channel->offTicks.ticks -= remainder;
    channel->setOCRegister(0);
    channel->handler = &handler_LowTimeContinue;
}

static void handler_LowTimeContinue(Channel* channel) {
    if (channel->offTicks.ticks <= 256) {
        channel->setOCRegister(channel->offTicks.ticksL);
        channel->handler = &handler_LowTimeFinish;
        return;
    }
    --channel->offTicks.ticksH;
}

static void handler_LowTimeFinish(Channel* channel) {
    channel->setCompareMatchInt(false);
    stopTimer();
    channel->finished = true;
}    
 
static void startTimer(void) {
    GTCCR_TSM = 1;
    GTCCR_PSR0 = 1;

    TCNT0 = 255;

    TCCR0B_CS02 = 1;
    TCCR0B_CS01 = 0;
    TCCR0B_CS00 = 1;

    GTCCR_TSM = 0;
}

static void stopTimer() {
    GTCCR_TSM = 1;
    GTCCR_PSR0 = 1;

    TCCR0B_CS02 = 0;
    TCCR0B_CS01 = 0;
    TCCR0B_CS00 = 0;

    GTCCR_TSM = 0;
}

static void setCompareMatchIntA(bool enable) {
    TIFR_OCF0A = 1;
    TIMSK_OCIE0A = enable;
}

static void setCompareMatchIntB(bool enable) {
    TIFR_OCF0B = 1;
    TIMSK_OCIE0B = enable;
}

static void setOutLowOnIntA(void) {
    TCCR0A_COM0A0 = 0;
    TCCR0A_COM0A1 = 1;
}

static void setOutHighOnIntA(void) {
    TCCR0A_COM0A0 = 1;
    TCCR0A_COM0A1 = 1;
}

static void setOutLowOnIntB(void) {
    TCCR0A_COM0B0 = 0;
    TCCR0A_COM0B1 = 1;
}

static void setOutHighOnIntB(void) {
    TCCR0A_COM0B0 = 1;
    TCCR0A_COM0B1 = 1;
}

static void setOCRegisterA(uint8_t value) {
    OCR0A = value;
}

static void setOCRegisterB(uint8_t value) {
    OCR0B = value;
}

#pragma vector=TIM0_COMPA_vect
__interrupt void timer0_CompA(void)
{
    channelA.handler(&channelA);
}

#pragma vector=TIM0_COMPB_vect
__interrupt void timer0_CompB(void)
{
    channelB.handler(&channelB);
}
