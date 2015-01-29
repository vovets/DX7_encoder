#pragma once

typedef void (*EncCallback)(void);

void encInit(void);
void encSetCWCallback(EncCallback);
void encSetCCWCallback(EncCallback);
