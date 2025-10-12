// 17:57 8 Oct 2025
#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include "stepper_commands.h"
#include "stepper_helpers.h"
#include "circular_buffer.h"

// Replace with your GUI MAC address (update to your GUI device)
const uint8_t GUI_MAC[] = { 0x98, 0xA3, 0x16, 0xE3, 0xFD, 0x4C };

// Command buffer for deferring work from ISR/callback to loop()
CircularBuffer<Message, 16> cb;

// Optional: log helper
static void printMac(const uint8_t *mac)
{
  for (int i = 0; i < 6; ++i) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
}

// ESP-NOW receive callback (older Arduino core signature used by PlatformIO)
void onDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
  // Note: stepperGUI uses newer signature with esp_now_recv_info_t
  // but PlatformIO uses older signature with direct MAC parameter
  
  Serial.print("onDataRecv called from ");
  if (mac_addr) {
    printMac(mac_addr);
    Serial.println();
  } else {
    Serial.println("(no mac)");
  }

  if (!incomingData || len < (int)sizeof(Message)) {
    Serial.printf("Received too few bytes: %d (need %u)\n", len, (unsigned)sizeof(Message));
    return;
  }

  Message msg;
  memcpy(&msg, incomingData, sizeof(msg));

  Serial.print("MessageId: "); Serial.print(msg.messageId);
  Serial.print(" cmd="); Serial.print((int)msg.command);
  Serial.print(" param="); Serial.println(msg.param);

  // Enqueue for processing in loop()
  if (!cb.push(msg)) {
    Serial.println("Command buffer full, dropping incoming message");
  }

  // Send ACK back to sender (use same sender MAC)
  Message ack{};
  ack.messageId = msg.messageId;
  ack.command = CMD_ACK;
  ack.param = 0;
  esp_err_t r = esp_now_send(mac_addr, (uint8_t *)&ack, sizeof(ack));
  if (r == ESP_OK) Serial.println("ACK sent");
  else Serial.printf("ACK send failed: %d\n", r);
}

// Optional send callback for logging (older signature for PlatformIO compatibility)
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("onDataSent to ");
  if (mac_addr) printMac(mac_addr);
  Serial.print(" status=");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

// =====================
// Hardware Pin Definitions
// =====================
constexpr int DIR_PIN = 18;
constexpr int STEP_PIN = 19;
constexpr int DOWN_LIMIT_PIN = 22;
constexpr int UP_LIMIT_PIN = 23;

// =====================
// Timing and Speed
// =====================
constexpr long SLOW_PD = 40;
constexpr long MEDIUM_PD = 20;
constexpr long FAST_PD = 10;
long PD = 100;
constexpr int LOOP_DELAY = 5;
constexpr int STEPPER_POSITION_MIN = 0;
constexpr int STEPPER_POSITION_MAX = 16000;
// Throttle for position sends (ms)
const unsigned long SEND_INTERVAL_MS = 100;
unsigned long lastSendTime = 0;

// Limit switch simulation for testing (set to true to enable)
const bool SIMULATE_LIMIT_SWITCHES = false;
unsigned long lastSimulationTime = 0;
const unsigned long SIMULATION_INTERVAL_MS = 1000; // 1 second
bool simulated_up_limit = false;
bool simulated_down_limit = false;

// =====================
// Stepper State Machine
// =====================
enum StepperState {
  STATE_IDLE,
  STATE_MOVING_UP,
  STATE_MOVING_DOWN,
  STATE_MOVING_TO,
  STATE_MOVE_TO_DOWN_LIMIT,
  STATE_RESETTING
};
StepperState state = STATE_IDLE;
int moveTarget = 0;

volatile int position = 0;
volatile bool Stop = true;
bool Direction = true; // true = up

// Helper to send Message to GUI (uses GUI_MAC defined earlier)
void sendMessage(CommandType cmd, int32_t param = STEPPER_PARAM_UNUSED, uint8_t messageId = 0)
{
  Message msg;
  msg.messageId = messageId;
  msg.command = cmd;
  msg.param = param;
  esp_err_t r = esp_now_send(GUI_MAC, (uint8_t *)&msg, sizeof(msg));
  if (r != ESP_OK) Serial.printf("esp_now_send failed: %d\n", r);
}

// Limit switches (with optional simulation for testing)
bool up_limit_tripped() { 
  if (SIMULATE_LIMIT_SWITCHES) {
    return simulated_up_limit;
  }
  return digitalRead(UP_LIMIT_PIN) == LOW; 
}

bool down_limit_tripped() { 
  if (SIMULATE_LIMIT_SWITCHES) {
    return simulated_down_limit;
  }
  return digitalRead(DOWN_LIMIT_PIN) == LOW; 
}

// Simulate limit switch state changes for testing
void update_limit_simulation() {
  if (!SIMULATE_LIMIT_SWITCHES) return;
  
  unsigned long now = millis();
  if (now - lastSimulationTime >= SIMULATION_INTERVAL_MS) {
    // Cycle through different limit switch states
    static int sim_state = 0;
    
    switch (sim_state) {
      case 0: // Both limits OK
        simulated_up_limit = false;
        simulated_down_limit = false;
        Serial.println("SIMULATION: Both limits OK");
        break;
      case 1: // Up limit tripped
        simulated_up_limit = true;
        simulated_down_limit = false;
        Serial.println("SIMULATION: Up limit TRIPPED");
        break;
      case 2: // Down limit tripped
        simulated_up_limit = false;
        simulated_down_limit = true;
        Serial.println("SIMULATION: Down limit TRIPPED");
        break;
      case 3: // Both limits tripped (unusual but possible)
        simulated_up_limit = true;
        simulated_down_limit = true;
        Serial.println("SIMULATION: Both limits TRIPPED");
        break;
    }
    
    // Send limit status updates to GUI
    sendMessage(simulated_up_limit ? CMD_UP_LIMIT_TRIP : CMD_UP_LIMIT_OK, STEPPER_PARAM_UNUSED);
    sendMessage(simulated_down_limit ? CMD_DOWN_LIMIT_TRIP : CMD_DOWN_LIMIT_OK, STEPPER_PARAM_UNUSED);
    
    sim_state = (sim_state + 1) % 4; // Cycle through 0-3
    lastSimulationTime = now;
  }
}

// Safety check for movement direction
bool movement_safe(bool direction) {
  if (direction && up_limit_tripped()) {
    return false; // Can't move up - up limit tripped
  }
  if (!direction && down_limit_tripped()) {
    return false; // Can't move down - down limit tripped
  }
  return true; // Movement is safe
}

// Log current limit switch status
void log_limit_status() {
  Serial.print("Limit switches: UP=");
  Serial.print(up_limit_tripped() ? "TRIPPED" : "OK");
  Serial.print(", DOWN=");
  Serial.println(down_limit_tripped() ? "TRIPPED" : "OK");
}

// Movement primitives
void single_step(long pulseDelay, bool dir)
{
  digitalWrite(DIR_PIN, dir);
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(pulseDelay);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(pulseDelay);
  if (dir) position++;
  else position--;
  position = constrain(position, STEPPER_POSITION_MIN, STEPPER_POSITION_MAX);
}

// Non-blocking stepping state: use micros() to schedule steps so loop() stays responsive
unsigned long lastStepMicros = 0;

// stepPeriodMicros is derived from PD (two pulse halves). We compute it each loop to reflect PD changes.
static inline unsigned long stepPeriodFromPD(long pulseDelay) {
  // single_step uses delayMicroseconds(pulseDelay) twice -> total ~2*pulseDelay
  unsigned long period = (unsigned long)(2UL * (unsigned long)max(1L, pulseDelay));
  return period;
}

// Initialize moveTo operation (called once when CMD_MOVE_TO is received)
void startMoveTo(int pos) {
  int difference = pos - position;
  
  Serial.print("Position = "); Serial.print(position); 
  Serial.print("     move to = "); Serial.print(pos); 
  Serial.print("   Diff = "); Serial.println(difference);
  
  if (difference == 0) {
    // Already at target position
    Serial.println("Already at target position");
    Stop = true;
    state = STATE_IDLE;
    sendMessage(CMD_POSITION, position);
  } else if (difference > 0) {
    // Need to move up
    Serial.println(" case 1 - will move up");
    moveTarget = pos;
    state = STATE_MOVING_TO;
    Stop = false;
  } else {
    // Need to move down  
    Serial.println(" case -1 - will move down");
    moveTarget = pos;
    state = STATE_MOVING_TO;
    Stop = false;
  }
}

// Command handler
void handle_command(const Message &msg)
{
  switch (msg.command) {
    case CMD_STOP:
      Stop = true;
      state = STATE_IDLE;
      sendMessage(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
      break;
    case CMD_UP_SLOW:
      if (up_limit_tripped()) {
        Serial.println("UP movement blocked - up limit switch tripped");
        sendMessage(CMD_UP_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
      } else {
        PD = SLOW_PD; Direction = true; Stop = false; state = STATE_MOVING_UP;
        sendMessage(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
      }
      break;
    case CMD_UP_MEDIUM:
      if (up_limit_tripped()) {
        Serial.println("UP movement blocked - up limit switch tripped");
        sendMessage(CMD_UP_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
      } else {
        PD = MEDIUM_PD; Direction = true; Stop = false; state = STATE_MOVING_UP;
        sendMessage(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
      }
      break;
    case CMD_UP_FAST:
      if (up_limit_tripped()) {
        Serial.println("UP movement blocked - up limit switch tripped");
        sendMessage(CMD_UP_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
      } else {
        PD = FAST_PD; Direction = true; Stop = false; state = STATE_MOVING_UP;
        sendMessage(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
      }
      break;
    case CMD_DOWN_SLOW:
      if (down_limit_tripped()) {
        Serial.println("DOWN movement blocked - down limit switch tripped");
        sendMessage(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
      } else {
        PD = SLOW_PD; Direction = false; Stop = false; state = STATE_MOVING_DOWN;
        sendMessage(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
      }
      break;
    case CMD_DOWN_MEDIUM:
      if (down_limit_tripped()) {
        Serial.println("DOWN movement blocked - down limit switch tripped");
        sendMessage(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
      } else {
        PD = MEDIUM_PD; Direction = false; Stop = false; state = STATE_MOVING_DOWN;
        sendMessage(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
      }
      break;
    case CMD_DOWN_FAST:
      if (down_limit_tripped()) {
        Serial.println("DOWN movement blocked - down limit switch tripped");
        sendMessage(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
      } else {
        PD = FAST_PD; Direction = false; Stop = false; state = STATE_MOVING_DOWN;
        sendMessage(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
      }
      break;
    case CMD_MOVE_TO:
      moveTarget = constrain((int)msg.param, STEPPER_POSITION_MIN, STEPPER_POSITION_MAX);
      
      // Check if movement direction would hit a limit switch
      if (moveTarget > position && up_limit_tripped()) {
        Serial.println("MOVE_TO blocked - target requires UP movement but up limit tripped");
        sendMessage(CMD_UP_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
      } else if (moveTarget < position && down_limit_tripped()) {
        Serial.println("MOVE_TO blocked - target requires DOWN movement but down limit tripped");
        sendMessage(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED, msg.messageId);
      } else {
        PD = FAST_PD; // Use fast speed for move to operations
        sendMessage(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
        startMoveTo(moveTarget); // Initialize non-blocking moveTo operation
      }
      break;
    case CMD_MOVE_TO_DOWN_LIMIT:
      Stop = false; state = STATE_MOVE_TO_DOWN_LIMIT;
      sendMessage(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
      break;
    case CMD_DOWN_LIMIT_STATUS:
      sendMessage(down_limit_tripped() ? CMD_DOWN_LIMIT_TRIP : CMD_DOWN_LIMIT_OK, STEPPER_PARAM_UNUSED, msg.messageId);
      break;
    case CMD_REQUEST_DOWN_STOP:
      sendMessage(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
      break;
    case CMD_GET_POSITION:
      sendMessage(CMD_GET_POSITION, position, msg.messageId);
      break;
    case CMD_RESET:
      Stop = true; 
      state = STATE_RESETTING; 
      sendMessage(CMD_ACK, STEPPER_PARAM_UNUSED, msg.messageId);
      Serial.println("Reset command received - restarting controller...");
      delay(100); // Brief delay to ensure ACK is sent
      ESP.restart();
      break;
    default:
      // Unknown command
      break;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(300);
  Serial.println("StepperController starting...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  Serial.printf("My MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }
  Serial.println("ESP-NOW Initialized");

  // Configure stepper pins and limit switches
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DOWN_LIMIT_PIN, INPUT_PULLUP);
  pinMode(UP_LIMIT_PIN, INPUT_PULLUP);

  // Register callbacks (old-style signatures expected by this core)
  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);

  // Add GUI as peer (optional but helpful)
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, GUI_MAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("Failed to add GUI as peer");
  }

  Serial.println("Setup done");
  
  // Log simulation status
  if (SIMULATE_LIMIT_SWITCHES) {
    Serial.println("*** LIMIT SWITCH SIMULATION ENABLED ***");
    Serial.println("Switches will change state every 1 second for GUI testing");
  } else {
    Serial.println("Using physical limit switches");
  }
  
  // Log initial limit switch status
  log_limit_status();
  Serial.print("Initial position: ");
  Serial.println(position);
  
  // Send reset command to GUI after controller startup
  delay(2000); // Wait for system to stabilize and GUI to be ready
  Serial.println("Sending reset command to GUI...");
  sendMessage(CMD_RESET, STEPPER_PARAM_UNUSED, 0);
  delay(500); // Give time for message to be sent
}

void loop()
{
  // Update limit switch simulation for testing
  update_limit_simulation();
  
  // Process queued messages
  Message msg;
  while (cb.pop(msg)) {
    Serial.print("Processing msg id="); Serial.print(msg.messageId);
    Serial.print(" cmd="); Serial.print(commandToString(msg.command));
    Serial.print(" param="); Serial.println(msg.param);
    handle_command(msg);
  }

  // Non-blocking state machine stepping
  unsigned long nowMicros = micros();
  unsigned long stepPeriod = stepPeriodFromPD(PD);

  switch (state) {
    case STATE_MOVING_UP:
      if (Stop || up_limit_tripped()) {
        Stop = true;
        state = STATE_IDLE;
        sendMessage(up_limit_tripped() ? CMD_UP_LIMIT_TRIP : CMD_UP_LIMIT_OK, STEPPER_PARAM_UNUSED);
        break;
      }
      if (nowMicros - lastStepMicros >= stepPeriod) {
        single_step(PD, true);
        lastStepMicros = nowMicros;
        if (millis() - lastSendTime >= SEND_INTERVAL_MS) {
          sendMessage(CMD_POSITION, position);
          lastSendTime = millis();
        }
      }
      break;

    case STATE_MOVING_DOWN:
      if (Stop || down_limit_tripped()) {
        Stop = true;
        state = STATE_IDLE;
        sendMessage(down_limit_tripped() ? CMD_DOWN_LIMIT_TRIP : CMD_DOWN_LIMIT_OK, STEPPER_PARAM_UNUSED);
        break;
      }
      if (nowMicros - lastStepMicros >= stepPeriod) {
        single_step(PD, false);
        lastStepMicros = nowMicros;
        if (millis() - lastSendTime >= SEND_INTERVAL_MS) {
          sendMessage(CMD_POSITION, position);
          lastSendTime = millis();
        }
      }
      break;

    case STATE_MOVING_TO:
      if (Stop || up_limit_tripped() || down_limit_tripped()) {
        Stop = true; state = STATE_IDLE; sendMessage(CMD_POSITION, position);
        break;
      }
      if (position == moveTarget) {
        Stop = true; state = STATE_IDLE; 
        sendMessage(CMD_POSITION, position);
        Serial.print("Move To completed. Final position = "); Serial.println(position);
        break;
      }
      if (nowMicros - lastStepMicros >= stepPeriod) {
        bool dir = (moveTarget > position);
        single_step(PD, dir);
        lastStepMicros = nowMicros;
        if (millis() - lastSendTime >= SEND_INTERVAL_MS) {
          sendMessage(CMD_POSITION, position);
          lastSendTime = millis();
        }
      }
      break;

    case STATE_MOVE_TO_DOWN_LIMIT:
      if (Stop || down_limit_tripped()) {
        Stop = true; state = STATE_IDLE; position = STEPPER_POSITION_MIN; sendMessage(CMD_DOWN_LIMIT_TRIP, STEPPER_PARAM_UNUSED);
        break;
      }
      if (nowMicros - lastStepMicros >= stepPeriod) {
        single_step(PD, false);
        lastStepMicros = nowMicros;
        if (millis() - lastSendTime >= SEND_INTERVAL_MS) {
          sendMessage(CMD_POSITION, position);
          lastSendTime = millis();
        }
      }
      break;

    case STATE_RESETTING:
    case STATE_IDLE:
    default:
      // idle: nothing to do
      break;
  }

  // Small yield to avoid busy loop
  delay(1);
}