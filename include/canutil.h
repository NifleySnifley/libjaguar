#include "can.h"

#ifndef CANUTIL_H
#define CANUTIL_H

#include <math.h>


int encode_can_message(CANMessage* message, CANEncodedMsg* encoded_message);
int decode_can_message(CANEncodedMsg* encoded_message, CANMessage* message);

float fixed16_to_float(uint16_t fx);
float fixed32_to_float(uint32_t fx);
uint16_t percent_to_fixed16(float f);
uint16_t float_to_fixed16(float f);
uint32_t float_to_fixed32(float f);

#endif
