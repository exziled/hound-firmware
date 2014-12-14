#ifndef __HOUND_ALIGN_H
#define __HOUND_ALIGN_H

#include <stdint.h>
#include <cstddef>

#include "hound_fixed.h"

int16_t alignHOUND_currentReference(uint32_t bits, bool reSample = false);
int16_t alignHOUND_HallEffectGetOffset(uint8_t socket_id);
fixed_t alignHOUND_HallEffectSetOffset(uint8_t socket_id, fixed_t * currentBuffer, int16_t bufferSize);

#endif //__HOUND_ALIGN_H