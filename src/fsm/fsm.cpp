#include "fsm.h"
#include "stepper_helpers.h"
#include "send_message_bridge.h"
#include <Preferences.h>

extern void send_message(CommandType cmd, int32_t param, uint8_t messageId = 0);
extern bool up_limit_tripped();
extern bool down_limit_tripped();
extern void single_step(long pulse_delay, bool dir);
extern unsigned long last_step_micros;
extern unsigned long last_send_time;
extern const unsigned long SEND_INTERVAL_MS;
extern const int STEPPER_POSITION_MIN;
extern const int STEPPER_POSITION_MAX;
extern long slow_pd, med_pd, fast_pd, moveto_pd, pd;
extern Preferences prefs;

void fsm_init(StepperContext *ctx) {
    ctx->state = STATE_IDLE;
    ctx->move_target = 0;
    ctx->position = 0;
    ctx->pd = slow_pd;
    ctx->stop_flag = true;
    ctx->direction = true;
}

static void fsm_start_move_to(StepperContext *ctx, int pos) {
    int difference = pos - ctx->position;
    Serial.print("Position = "); Serial.print(ctx->position);
    Serial.print("     move to = "); Serial.print(pos);
    Serial.print("   Diff = "); Serial.println(difference);
    if (difference == 0) {
        Serial.println("Already at target position");
        ctx->stop_flag = true;
        ctx->state = STATE_IDLE;
        send_message(CMD_POSITION, ctx->position);
    } else {
        ctx->move_target = pos;
        ctx->state = STATE_MOVING_TO;
        ctx->stop_flag = false;
    }
}

void fsm_handle_command(StepperContext *ctx, const Message &msg) {
    switch (msg.command) {
        case CMD_STOP:
            ctx->stop_flag = true;
            ctx->state = STATE_IDLE;
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_UP_SLOW:
            if (up_limit_tripped()) {
                send_message(CMD_UP_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                ctx->pd = slow_pd; ctx->direction = true; ctx->stop_flag = false; ctx->state = STATE_MOVING_UP;
                send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            }
            break;
        case CMD_UP_MEDIUM:
            if (up_limit_tripped()) {
                send_message(CMD_UP_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                ctx->pd = med_pd; ctx->direction = true; ctx->stop_flag = false; ctx->state = STATE_MOVING_UP;
                send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            }
            break;
        case CMD_UP_FAST:
            if (up_limit_tripped()) {
                send_message(CMD_UP_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                ctx->pd = fast_pd; ctx->direction = true; ctx->stop_flag = false; ctx->state = STATE_MOVING_UP;
                send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            }
            break;
        case CMD_DOWN_SLOW:
            if (down_limit_tripped()) {
                send_message(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                ctx->pd = slow_pd; ctx->direction = false; ctx->stop_flag = false; ctx->state = STATE_MOVING_DOWN;
                send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            }
            break;
        case CMD_DOWN_MEDIUM:
            if (down_limit_tripped()) {
                send_message(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                ctx->pd = med_pd; ctx->direction = false; ctx->stop_flag = false; ctx->state = STATE_MOVING_DOWN;
                send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            }
            break;
        case CMD_DOWN_FAST:
            if (down_limit_tripped()) {
                send_message(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                ctx->pd = fast_pd; ctx->direction = false; ctx->stop_flag = false; ctx->state = STATE_MOVING_DOWN;
                send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            }
            break;
        case CMD_MOVE_TO:
            ctx->move_target = constrain((int)msg.param, STEPPER_POSITION_MIN, STEPPER_POSITION_MAX);
            if (ctx->move_target > ctx->position && up_limit_tripped()) {
                send_message(CMD_UP_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else if (ctx->move_target < ctx->position && down_limit_tripped()) {
                send_message(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                ctx->pd = fast_pd;
                send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
                fsm_start_move_to(ctx, ctx->move_target);
            }
            break;
        case CMD_MOVE_TO_DOWN_LIMIT:
            ctx->stop_flag = false; ctx->state = STATE_MOVE_TO_DOWN_LIMIT;
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_DOWN_LIMIT_STATUS:
            send_message(down_limit_tripped() ? CMD_DOWN_LIMIT_TRIP : CMD_DOWN_LIMIT_OK, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_REQUEST_DOWN_STOP:
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_GET_POSITION:
            send_message(CMD_GET_POSITION, ctx->position, msg.messageId);
            break;
        case CMD_RESET:
            ctx->stop_flag = true;
            ctx->state = STATE_RESETTING;
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            Serial.println("Reset command received - restarting controller...");
            delay(100);
            ESP.restart();
            break;
        default:
            // Unknown command
            break;
    }
}

void fsm_handle(StepperContext *ctx, unsigned long now_micros, unsigned long step_period) {
    static int last_sent_position = -999999;
    bool moving = (ctx->state == STATE_MOVING_UP || ctx->state == STATE_MOVING_DOWN || ctx->state == STATE_MOVING_TO || ctx->state == STATE_MOVE_TO_DOWN_LIMIT);
    if (moving && ctx->position != last_sent_position) {
        send_message(CMD_POSITION, ctx->position);
        last_sent_position = ctx->position;
    }

    switch (ctx->state) {
        case STATE_MOVING_UP:
            if (ctx->stop_flag || up_limit_tripped()) {
                ctx->stop_flag = true;
                ctx->state = STATE_IDLE;
                send_message(up_limit_tripped() ? CMD_UP_LIMIT_TRIP : CMD_UP_LIMIT_OK, STEPPER_PARAM_UNUSED);
                break;
            }
            if (now_micros - last_step_micros >= step_period) {
                single_step(ctx->pd, true);
                last_step_micros = now_micros;
            }
            break;
        case STATE_MOVING_DOWN:
            if (ctx->stop_flag || down_limit_tripped()) {
                ctx->stop_flag = true;
                ctx->state = STATE_IDLE;
                send_message(down_limit_tripped() ? CMD_DOWN_LIMIT_TRIP : CMD_DOWN_LIMIT_OK, STEPPER_PARAM_UNUSED);
                break;
            }
            if (now_micros - last_step_micros >= step_period) {
                single_step(ctx->pd, false);
                last_step_micros = now_micros;
            }
            break;
        case STATE_MOVING_TO:
            if (ctx->stop_flag || up_limit_tripped() || down_limit_tripped()) {
                ctx->stop_flag = true;
                ctx->state = STATE_IDLE;
                send_message(CMD_POSITION, ctx->position);
                break;
            }
            if (ctx->position == ctx->move_target) {
                ctx->stop_flag = true;
                ctx->state = STATE_IDLE;
                send_message(CMD_POSITION, ctx->position);
                break;
            }
            if (now_micros - last_step_micros >= step_period) {
                bool dir = (ctx->move_target > ctx->position);
                single_step(ctx->pd, dir);
                last_step_micros = now_micros;
            }
            break;
        case STATE_MOVE_TO_DOWN_LIMIT:
            if (ctx->stop_flag || down_limit_tripped()) {
                ctx->stop_flag = true;
                ctx->state = STATE_IDLE;
                ctx->position = STEPPER_POSITION_MIN;
                send_message(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED);
                break;
            }
            if (now_micros - last_step_micros >= step_period) {
                single_step(ctx->pd, false);
                last_step_micros = now_micros;
            }
            break;
        case STATE_RESETTING:
        case STATE_IDLE:
        default:
            // idle: nothing to do
            break;
    }
}
