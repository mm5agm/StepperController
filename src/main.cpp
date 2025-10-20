// 17:57 8 Oct 2025
#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include "stepper_commands.h"
#include "stepper_helpers.h"
#include "circular_buffer.h"
#include "fsm/fsm.h"
#include "commandToString.h"
#include "send_message_bridge.h"
StepperContext fsm_ctx;
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
long slow_pd = SLOW_PD;
long med_pd = MEDIUM_PD;
long fast_pd = FAST_PD;
long moveto_pd = FAST_PD;
long PD = 100;
constexpr int LOOP_DELAY = 5;
extern const int STEPPER_POSITION_MIN = 0;
extern const int STEPPER_POSITION_MAX = 16000;
// Throttle for position sends (ms)
const unsigned long SEND_INTERVAL_MS = 100;
unsigned long lastSendTime = 0;

// Limit switch simulation for testing (set to true to enable)
const bool SIMULATE_LIMIT_SWITCHES = false;
unsigned long lastSimulationTime = 0;
const unsigned long SIMULATION_INTERVAL_MS = 1000; // 1 second
bool simulated_up_limit = false;
bool simulated_down_limit = false;


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
  // Update position via FSM context
  if (dir) fsm_ctx.position++;
  else fsm_ctx.position--;
  fsm_ctx.position = constrain(fsm_ctx.position, STEPPER_POSITION_MIN, STEPPER_POSITION_MAX);
}

// Non-blocking stepping state: use micros() to schedule steps so loop() stays responsive
unsigned long lastStepMicros = 0;
unsigned long last_step_micros = 0;

// stepPeriodMicros is derived from PD (two pulse halves). We compute it each loop to reflect PD changes.
static inline unsigned long stepPeriodFromPD(long pulseDelay) {
  // single_step uses delayMicroseconds(pulseDelay) twice -> total ~2*pulseDelay
  unsigned long period = (unsigned long)(2UL * (unsigned long)max(1L, pulseDelay));
  return period;
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
  fsm_init(&fsm_ctx);
Serial.print("Initial position: ");
Serial.println(fsm_ctx.position);
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
  Message msg;
while (cb.pop(msg)) {
  Serial.print("Processing msg id="); Serial.print(msg.messageId);
  Serial.print(" cmd="); Serial.print(commandToString(msg.command));
  Serial.print(" param="); Serial.println(msg.param);
  fsm_handle_command(&fsm_ctx, msg);
}

unsigned long now_micros = micros();
unsigned long step_period = stepPeriodFromPD(PD);
fsm_handle(&fsm_ctx, now_micros, step_period);

delay(1);
  
}

// Bridge for FSM to call main's sendMessage
#ifdef __cplusplus
extern "C" {
#endif
void send_message(CommandType cmd, int32_t param, uint8_t messageId) {
    Serial.print("[SEND] cmd: ");
    Serial.print(commandToString(cmd));
    Serial.print(" param: ");
    Serial.print(param);
    Serial.print(" msgId: ");
    Serial.println(messageId);
    sendMessage(cmd, param, messageId);
}
#ifdef __cplusplus
}
#endif