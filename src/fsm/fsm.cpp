#define D0_PIN 2
#include "fsm.h"
#include "stepper_helpers.h"
#include <Preferences.h>


extern void send_message(CommandType cmd, int32_t param, uint8_t messageId = 0);
// ...existing code...
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

    // Initialize TCRT5000 digital output pin
    pinMode(2, INPUT);
}

// Helper for move-to operation
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
    // All case labels must be inside the switch below
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
            ctx->stop_flag = true;
            ctx->state = STATE_IDLE;
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_HOME:
            Serial.println("[FSM] CMD_HOME received");
            // Start homing procedure here
            ctx->state = STATE_MOVE_TO_HOME;
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_SENSOR_STATUS:
            Serial.println("[FSM] CMD_SENSOR_STATUS received");
            // Read and report sensor status (e.g., TCRT5000)
            send_message(CMD_SENSOR_STATUS, digitalRead(2), msg.messageId);
            break;
        case CMD_MOVE_TO_HOME:
            Serial.println("[FSM] CMD_MOVE_TO_HOME received");
            // Move down until TCRT5000 sensor detects the white mark (LOW on pin 2)
            ctx->stop_flag = false;
            ctx->state = STATE_MOVE_TO_HOME;
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_HOME_COMPLETE:
            Serial.println("[FSM] CMD_HOME_COMPLETE received");
            send_message(CMD_HOME_COMPLETE, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_HOME_FAILED:
            Serial.println("[FSM] CMD_HOME_FAILED received");
            send_message(CMD_HOME_FAILED, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_SENSOR_ERROR:
            Serial.println("[FSM] CMD_SENSOR_ERROR received");
            send_message(CMD_SENSOR_ERROR, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
            break;
        case CMD_UP_SLOW:
            Serial.println("[FSM] CMD_UP_SLOW received");
            pd = slow_pd; ctx->direction = true; ctx->stop_flag = false; ctx->state = STATE_MOVING_UP;
            Serial.println("[FSM] Moving UP (slow): pd set, direction=true, stop_flag=false, state=MOVING_UP");
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_UP_MEDIUM:
            Serial.println("[FSM] CMD_UP_MEDIUM received");
            pd = med_pd; ctx->direction = true; ctx->stop_flag = false; ctx->state = STATE_MOVING_UP;
            Serial.println("[FSM] Moving UP (medium): pd set, direction=true, stop_flag=false, state=MOVING_UP");
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_UP_FAST:
            Serial.println("[FSM] CMD_UP_FAST received");
            pd = fast_pd; ctx->direction = true; ctx->stop_flag = false; ctx->state = STATE_MOVING_UP;
            Serial.println("[FSM] Moving UP (fast): pd set, direction=true, stop_flag=false, state=MOVING_UP");
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_DOWN_SLOW:
            Serial.println("[FSM] CMD_DOWN_SLOW received");
            pd = slow_pd; ctx->direction = false; ctx->stop_flag = false; ctx->state = STATE_MOVING_DOWN;
            Serial.println("[FSM] Moving DOWN (slow): pd set, direction=false, stop_flag=false, state=MOVING_DOWN");
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_DOWN_MEDIUM:
            Serial.println("[FSM] CMD_DOWN_MEDIUM received");
            pd = med_pd; ctx->direction = false; ctx->stop_flag = false; ctx->state = STATE_MOVING_DOWN;
            Serial.println("[FSM] Moving DOWN (medium): pd set, direction=false, stop_flag=false, state=MOVING_DOWN");
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_DOWN_FAST:
            Serial.println("[FSM] CMD_DOWN_FAST received");
            pd = fast_pd; ctx->direction = false; ctx->stop_flag = false; ctx->state = STATE_MOVING_DOWN;
            Serial.println("[FSM] Moving DOWN (fast): pd set, direction=false, stop_flag=false, state=MOVING_DOWN");
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            break;
        case CMD_MOVE_TO:
            Serial.print("[FSM] CMD_MOVE_TO received, param=");
            Serial.println(msg.param);
            ctx->move_target = constrain((int)msg.param, STEPPER_POSITION_MIN, STEPPER_POSITION_MAX);
            pd = moveto_pd;
            Serial.print("[FSM] Starting move-to: target=");
            Serial.println(ctx->move_target);
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            fsm_start_move_to(ctx, ctx->move_target);
            break;
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
        // ...existing code...
        case CMD_GET_POSITION:
            send_message(CMD_GET_POSITION, ctx->position, msg.messageId);
            break;
        case CMD_RESET:
            ctx->stop_flag = true;
            ctx->state = STATE_RESETTING;
            send_message(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
            Serial.println("[DEBUG] CMD_RESET received: preparing to restart controller...");
            delay(100);
            Serial.println("[DEBUG] Calling ESP.restart() now...");
            ESP.restart();
            Serial.println("[DEBUG] ESP.restart() returned (should not happen)");
            break;
        default:
            // Unknown command
            break;
    }
}

void fsm_handle(StepperContext *ctx, unsigned long now_micros, unsigned long step_period) {
    switch (ctx->state) {
        case STATE_MOVING_UP:
            if (ctx->stop_flag) {
                ctx->stop_flag = true;
                ctx->state = STATE_IDLE;
                break;
            }
            if (now_micros - last_step_micros >= step_period) {
                single_step(pd, true);
                last_step_micros = now_micros;
                if (millis() - last_send_time >= SEND_INTERVAL_MS) {
                    send_message(CMD_POSITION, ctx->position);
                    last_send_time = millis();
                }
            }
            break;
        case STATE_MOVING_DOWN:
            if (ctx->stop_flag) {
                ctx->stop_flag = true;
                ctx->state = STATE_IDLE;
                break;
            }
            if (now_micros - last_step_micros >= step_period) {
                single_step(pd, false);
                last_step_micros = now_micros;
                if (millis() - last_send_time >= SEND_INTERVAL_MS) {
                    send_message(CMD_POSITION, ctx->position);
                    last_send_time = millis();
                }
            }
            break;
        case STATE_MOVING_TO:
            if (ctx->stop_flag) {
                ctx->stop_flag = true; ctx->state = STATE_IDLE; send_message(CMD_POSITION, ctx->position);
                break;
            }
            if (ctx->position == ctx->move_target) {
                ctx->stop_flag = true; ctx->state = STATE_IDLE;
                send_message(CMD_POSITION, ctx->position);
                Serial.print("Move To completed. Final position = "); Serial.println(ctx->position);
                break;
            }
            if (now_micros - last_step_micros >= step_period) {
                bool dir = (ctx->move_target > ctx->position);
                single_step(pd, dir);
                last_step_micros = now_micros;
                if (millis() - last_send_time >= SEND_INTERVAL_MS) {
                    send_message(CMD_POSITION, ctx->position);
                    last_send_time = millis();
                }
            }
            break;
        case STATE_MOVE_TO_HOME: {
            // Use TCRT5000 digital output on pin 2 for limit detection
            if (ctx->stop_flag) {
                ctx->stop_flag = true;
                ctx->state = STATE_IDLE;
                ctx->position = STEPPER_POSITION_MIN;
                break;
            }
            // Read TCRT5000 digital output (LOW = detected, HIGH = not detected)
            int tcrt5000_state = digitalRead(2);
            if (tcrt5000_state == LOW) { // Detected (reflective surface or object present)
                Serial.println("[FSM] TCRT5000 detected: at home (white mark)");
                ctx->stop_flag = true;
                ctx->state = STATE_IDLE;
                ctx->position = STEPPER_POSITION_MIN;
                send_message(CMD_HOME_COMPLETE, ctx->position); // Notify home complete
                break;
            }
            if (now_micros - last_step_micros >= step_period) {
                single_step(pd, false);
                last_step_micros = now_micros;
                if (millis() - last_send_time >= SEND_INTERVAL_MS) {
                    send_message(CMD_POSITION, ctx->position);
                    last_send_time = millis();
                }
            }
            break;
        }
        case STATE_RESETTING:
        case STATE_IDLE:
        default:
            // idle: nothing to do
            break;
    }
}
