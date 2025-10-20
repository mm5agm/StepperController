#pragma once
#include "stepper_commands.h"
#ifdef __cplusplus
extern "C" {
#endif
void send_message(CommandType cmd, int32_t param, uint8_t messageId);
#ifdef __cplusplus
}
#endif
