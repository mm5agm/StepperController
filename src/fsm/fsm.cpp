#include "fsm.h"
#include "stepper_helpers.h"
#include "send_message_bridge.h"
#include <Preferences.h>
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

<<<<<<< HEAD
void fsm_init(StepperContext *ctx) {
    ctx->state = STATE_IDLE;
    ctx->move_target = 0;
    ctx->position = 0;
    ctx->pd = slow_pd;
    ctx->stop_flag = true;
    ctx->direction = true;
}

=======
// Helper for move-to operation
>>>>>>> docs/nvs-migration
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
<<<<<<< HEAD
    switch (msg.command) {
        case CMD_STOP:
=======
    Serial.print("[FSM] Handling command: ");
    Serial.print(commandToString(msg.command));
    Serial.print(" param=");
    Serial.print(msg.param);
    Serial.print(" id=");
    Serial.println(msg.messageId);
    StepperState prev_state = ctx->state;
    bool prev_stop = ctx->stop_flag;
    int prev_pos = ctx->position;
    // Main FSM command switch
    switch (msg.command) {
        case CMD_STOP:
            Serial.println("[FSM] CMD_STOP received: setting stop_flag=true, state=IDLE");
>>>>>>> docs/nvs-migration
            ctx->stop_flag = true;
            ctx->state = STATE_IDLE;
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_UP_SLOW:
<<<<<<< HEAD
            if (up_limit_tripped()) {
                send_message(CMD_UP_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                ctx->pd = slow_pd; ctx->direction = true; ctx->stop_flag = false; ctx->state = STATE_MOVING_UP;
=======
            Serial.println("[FSM] CMD_UP_SLOW received");
            if (up_limit_tripped()) {
                Serial.println("[FSM] UP movement blocked - up limit switch tripped");
                send_message(CMD_UP_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                pd = slow_pd; ctx->direction = true; ctx->stop_flag = false; ctx->state = STATE_MOVING_UP;
                Serial.println("[FSM] Moving UP (slow): pd set, direction=true, stop_flag=false, state=MOVING_UP");
>>>>>>> docs/nvs-migration
                send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            }
            break;
        case CMD_UP_MEDIUM:
<<<<<<< HEAD
            if (up_limit_tripped()) {
                send_message(CMD_UP_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                ctx->pd = med_pd; ctx->direction = true; ctx->stop_flag = false; ctx->state = STATE_MOVING_UP;
=======
            Serial.println("[FSM] CMD_UP_MEDIUM received");
            if (up_limit_tripped()) {
                Serial.println("[FSM] UP movement blocked - up limit switch tripped");
                send_message(CMD_UP_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                pd = med_pd; ctx->direction = true; ctx->stop_flag = false; ctx->state = STATE_MOVING_UP;
                Serial.println("[FSM] Moving UP (medium): pd set, direction=true, stop_flag=false, state=MOVING_UP");
>>>>>>> docs/nvs-migration
                send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            }
            break;
        case CMD_UP_FAST:
<<<<<<< HEAD
            if (up_limit_tripped()) {
                send_message(CMD_UP_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                ctx->pd = fast_pd; ctx->direction = true; ctx->stop_flag = false; ctx->state = STATE_MOVING_UP;
=======
            Serial.println("[FSM] CMD_UP_FAST received");
            if (up_limit_tripped()) {
                Serial.println("[FSM] UP movement blocked - up limit switch tripped");
                send_message(CMD_UP_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                pd = fast_pd; ctx->direction = true; ctx->stop_flag = false; ctx->state = STATE_MOVING_UP;
                Serial.println("[FSM] Moving UP (fast): pd set, direction=true, stop_flag=false, state=MOVING_UP");
>>>>>>> docs/nvs-migration
                send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            }
            break;
        case CMD_DOWN_SLOW:
<<<<<<< HEAD
            if (down_limit_tripped()) {
                send_message(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                ctx->pd = slow_pd; ctx->direction = false; ctx->stop_flag = false; ctx->state = STATE_MOVING_DOWN;
=======
            Serial.println("[FSM] CMD_DOWN_SLOW received");
            if (down_limit_tripped()) {
                Serial.println("[FSM] DOWN movement blocked - down limit switch tripped");
                send_message(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                pd = slow_pd; ctx->direction = false; ctx->stop_flag = false; ctx->state = STATE_MOVING_DOWN;
                Serial.println("[FSM] Moving DOWN (slow): pd set, direction=false, stop_flag=false, state=MOVING_DOWN");
>>>>>>> docs/nvs-migration
                send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            }
            break;
        case CMD_DOWN_MEDIUM:
<<<<<<< HEAD
            if (down_limit_tripped()) {
                send_message(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                ctx->pd = med_pd; ctx->direction = false; ctx->stop_flag = false; ctx->state = STATE_MOVING_DOWN;
=======
            Serial.println("[FSM] CMD_DOWN_MEDIUM received");
            if (down_limit_tripped()) {
                Serial.println("[FSM] DOWN movement blocked - down limit switch tripped");
                send_message(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                pd = med_pd; ctx->direction = false; ctx->stop_flag = false; ctx->state = STATE_MOVING_DOWN;
                Serial.println("[FSM] Moving DOWN (medium): pd set, direction=false, stop_flag=false, state=MOVING_DOWN");
>>>>>>> docs/nvs-migration
                send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            }
            break;
        case CMD_DOWN_FAST:
<<<<<<< HEAD
            if (down_limit_tripped()) {
                send_message(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                ctx->pd = fast_pd; ctx->direction = false; ctx->stop_flag = false; ctx->state = STATE_MOVING_DOWN;
=======
            Serial.println("[FSM] CMD_DOWN_FAST received");
            if (down_limit_tripped()) {
                Serial.println("[FSM] DOWN movement blocked - down limit switch tripped");
                send_message(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                pd = fast_pd; ctx->direction = false; ctx->stop_flag = false; ctx->state = STATE_MOVING_DOWN;
                Serial.println("[FSM] Moving DOWN (fast): pd set, direction=false, stop_flag=false, state=MOVING_DOWN");
>>>>>>> docs/nvs-migration
                send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            }
            break;
        case CMD_MOVE_TO:
<<<<<<< HEAD
            ctx->move_target = constrain((int)msg.param, STEPPER_POSITION_MIN, STEPPER_POSITION_MAX);
            if (ctx->move_target > ctx->position && up_limit_tripped()) {
                send_message(CMD_UP_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else if (ctx->move_target < ctx->position && down_limit_tripped()) {
                send_message(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                ctx->pd = fast_pd;
=======
            Serial.print("[FSM] CMD_MOVE_TO received, param=");
            Serial.println(msg.param);
            ctx->move_target = constrain((int)msg.param, STEPPER_POSITION_MIN, STEPPER_POSITION_MAX);
            if (ctx->move_target > ctx->position && up_limit_tripped()) {
                Serial.println("[FSM] MOVE_TO blocked - target requires UP movement but up limit tripped");
                send_message(CMD_UP_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else if (ctx->move_target < ctx->position && down_limit_tripped()) {
                Serial.println("[FSM] MOVE_TO blocked - target requires DOWN movement but down limit tripped");
                send_message(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
            } else {
                pd = moveto_pd;
                Serial.print("[FSM] Starting move-to: target=");
                Serial.println(ctx->move_target);
>>>>>>> docs/nvs-migration
                send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
                fsm_start_move_to(ctx, ctx->move_target);
            }
            break;
<<<<<<< HEAD
=======
            // Print state change summary
            if (ctx->state != prev_state || ctx->stop_flag != prev_stop || ctx->position != prev_pos) {
                Serial.print("[FSM] State change: state=");
                Serial.print(ctx->state);
                Serial.print(" stop_flag=");
                Serial.print(ctx->stop_flag);
                Serial.print(" position=");
                Serial.println(ctx->position);
            }
        case CMD_SLOW_SPEED_PULSE_DELAY:
            slow_pd = max(1, (int)msg.param);
            prefs.putLong("slow_pd", slow_pd);
            Serial.print("Updated slow_pd to "); Serial.println(slow_pd);
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_MEDIUM_SPEED_PULSE_DELAY:
            med_pd = max(1, (int)msg.param);
            prefs.putLong("med_pd", med_pd);
            Serial.print("Updated med_pd to "); Serial.println(med_pd);
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_FAST_SPEED_PULSE_DELAY:
            fast_pd = max(1, (int)msg.param);
            prefs.putLong("fast_pd", fast_pd);
            Serial.print("Updated fast_pd to "); Serial.println(fast_pd);
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_MOVE_TO_PULSE_DELAY:
            moveto_pd = max(1, (int)msg.param);
            prefs.putLong("moveto_pd", moveto_pd);
            Serial.print("Updated moveto_pd to "); Serial.println(moveto_pd);
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
>>>>>>> docs/nvs-migration
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
<<<<<<< HEAD
    static int last_sent_position = -999999;
    bool moving = (ctx->state == STATE_MOVING_UP || ctx->state == STATE_MOVING_DOWN || ctx->state == STATE_MOVING_TO || ctx->state == STATE_MOVE_TO_DOWN_LIMIT);
    if (moving && ctx->position != last_sent_position) {
        send_message(CMD_POSITION, ctx->position);
        last_sent_position = ctx->position;
    }

=======
>>>>>>> docs/nvs-migration
    switch (ctx->state) {
        case STATE_MOVING_UP:
            if (ctx->stop_flag || up_limit_tripped()) {
                ctx->stop_flag = true;
                ctx->state = STATE_IDLE;
                send_message(up_limit_tripped() ? CMD_UP_LIMIT_TRIP : CMD_UP_LIMIT_OK, STEPPER_PARAM_UNUSED);
                break;
            }
            if (now_micros - last_step_micros >= step_period) {
<<<<<<< HEAD
                single_step(ctx->pd, true);
                last_step_micros = now_micros;
=======
                single_step(pd, true);
                last_step_micros = now_micros;
                if (millis() - last_send_time >= SEND_INTERVAL_MS) {
                    send_message(CMD_POSITION, ctx->position);
                    last_send_time = millis();
                }
>>>>>>> docs/nvs-migration
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
<<<<<<< HEAD
                single_step(ctx->pd, false);
                last_step_micros = now_micros;
=======
                single_step(pd, false);
                last_step_micros = now_micros;
                if (millis() - last_send_time >= SEND_INTERVAL_MS) {
                    send_message(CMD_POSITION, ctx->position);
                    last_send_time = millis();
                }
>>>>>>> docs/nvs-migration
            }
            break;
        case STATE_MOVING_TO:
            if (ctx->stop_flag || up_limit_tripped() || down_limit_tripped()) {
<<<<<<< HEAD
                ctx->stop_flag = true;
                ctx->state = STATE_IDLE;
                send_message(CMD_POSITION, ctx->position);
                break;
            }
            if (ctx->position == ctx->move_target) {
                ctx->stop_flag = true;
                ctx->state = STATE_IDLE;
                send_message(CMD_POSITION, ctx->position);
=======
                ctx->stop_flag = true; ctx->state = STATE_IDLE; send_message(CMD_POSITION, ctx->position);
                break;
            }
            if (ctx->position == ctx->move_target) {
                ctx->stop_flag = true; ctx->state = STATE_IDLE;
                send_message(CMD_POSITION, ctx->position);
                Serial.print("Move To completed. Final position = "); Serial.println(ctx->position);
>>>>>>> docs/nvs-migration
                break;
            }
            if (now_micros - last_step_micros >= step_period) {
                bool dir = (ctx->move_target > ctx->position);
<<<<<<< HEAD
                single_step(ctx->pd, dir);
                last_step_micros = now_micros;
=======
                single_step(pd, dir);
                last_step_micros = now_micros;
                if (millis() - last_send_time >= SEND_INTERVAL_MS) {
                    send_message(CMD_POSITION, ctx->position);
                    last_send_time = millis();
                }
>>>>>>> docs/nvs-migration
            }
            break;
        case STATE_MOVE_TO_DOWN_LIMIT:
            if (ctx->stop_flag || down_limit_tripped()) {
<<<<<<< HEAD
                ctx->stop_flag = true;
                ctx->state = STATE_IDLE;
                ctx->position = STEPPER_POSITION_MIN;
                send_message(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED);
                break;
            }
            if (now_micros - last_step_micros >= step_period) {
                single_step(ctx->pd, false);
                last_step_micros = now_micros;
=======
                ctx->stop_flag = true; ctx->state = STATE_IDLE; ctx->position = STEPPER_POSITION_MIN; send_message(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED);
                break;
            }
            if (now_micros - last_step_micros >= step_period) {
                single_step(pd, false);
                last_step_micros = now_micros;
                if (millis() - last_send_time >= SEND_INTERVAL_MS) {
                    send_message(CMD_POSITION, ctx->position);
                    last_send_time = millis();
                }
>>>>>>> docs/nvs-migration
            }
            break;
        case STATE_RESETTING:
        case STATE_IDLE:
        default:
            // idle: nothing to do
            break;
    }
}
