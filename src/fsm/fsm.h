
#pragma once
#include <stdint.h>
#include "stepper_commands.h"

// Stepper state machine states
enum StepperState {
    STATE_IDLE,
    STATE_MOVING_UP,
    STATE_MOVING_DOWN,
    STATE_MOVING_TO,
    STATE_MOVE_TO_HOME,
    STATE_RESETTING
};

// State machine context
struct StepperContext {
    StepperState state;
    int move_target;
    int position;
    long pd;
    bool stop_flag;
    bool direction;
};

// FSM API
void fsm_init(StepperContext *ctx);
void fsm_handle(StepperContext *ctx, unsigned long now_micros, unsigned long step_period);
void fsm_handle_command(StepperContext *ctx, const Message &msg);
