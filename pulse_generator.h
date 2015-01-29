#pragma once

#include <stdint.h>

void pgInit(void);
void pgStartChannelA(uint16_t onTicks, uint16_t offTicks);
void pgStartChannelB(uint16_t onTicks, uint16_t offTicks);
void pgWaitChannelA(void);
void pgWaitChannelB(void);

