// 17:57 8 Oct 2025
#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include "stepper_commands.h"
#include "stepper_helpers.h"
#include "circular_buffer.h"
#include <Preferences.h>
#include "fsm/fsm.h"

// Preferences for storing configuration across restarts
Preferences prefs;

// Replace with your GUI MAC address (update to your GUI device)
const uint8_t GUI_MAC[] = { 0x98, 0xA3, 0x16, 0xE3, 0xFD, 0x4C };

// Command buffer for deferring work from ISR/callback to loop()
CircularBuffer<Message, 16> cb;

// Optional: log helper
static void print_mac(const uint8_t *mac)
{
  for (int i = 0; i < 6; ++i) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
}

// Forward declarations: some helper functions are defined later in this file
// but are used earlier (e.g. in log_limit_status / update_limit_simulation).
// Provide default arguments here so earlier call sites (in this file) can use
// the two-argument form; the definition below must NOT repeat defaults.
bool up_limit_tripped();
bool down_limit_tripped();
void send_message(CommandType cmd, int32_t param = STEPPER_PARAM_UNUSED, uint8_t messageId = 0);

// ESP-NOW receive callback (older Arduino core signature used by PlatformIO)
void on_data_recv(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
  // Note: stepperGUI uses newer signature with esp_now_recv_info_t
  // but PlatformIO uses older signature with direct MAC parameter
  
  Serial.print("[RECEIVED] from ");
  if (mac_addr) {
    print_mac(mac_addr);
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

  Serial.print("[RECEIVED CMD] id="); Serial.print(msg.messageId);
  Serial.print(" cmd="); Serial.print(commandToString(msg.command));
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
  Serial.print("[SENT CMD] ACK to sender: id="); Serial.print(ack.messageId);
  Serial.print(" cmd="); Serial.print(commandToString(ack.command));
  Serial.print(" param="); Serial.println(ack.param);
  esp_err_t r = esp_now_send(mac_addr, (uint8_t *)&ack, sizeof(ack));
  if (r == ESP_OK) Serial.println("ACK sent");
  else Serial.printf("ACK send failed: %d\n", r);
}

// Optional send callback for logging (older signature for PlatformIO compatibility)
void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("onDataSent to ");
  if (mac_addr) print_mac(mac_addr);
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
// Runtime-configurable pulse delays (microseconds per half-pulse)
// Initialized to reasonable defaults; these may be overridden by Preferences or commands
long slow_pd = 40;
long med_pd = 20;
long fast_pd = 10;
// Pulse delay used for move-to operations (separately configurable)
long moveto_pd = 10;

long pd = 100;
constexpr int LOOP_DELAY = 5;
constexpr int STEPPER_POSITION_MIN = 0;
constexpr int STEPPER_POSITION_MAX = 16000;
// Throttle for position sends (ms)
const unsigned long SEND_INTERVAL_MS = 100;
unsigned long last_send_time = 0;

// Limit switch simulation for testing (set to true to enable)
const bool SIMULATE_LIMIT_SWITCHES = false;
unsigned long last_simulation_time = 0;
const unsigned long SIMULATION_INTERVAL_MS = 1000; // 1 second
bool simulated_up_limit = false;
bool simulated_down_limit = false;

// =====================
// FSM Context
// =====================
StepperContext fsm_ctx;
volatile int last_printed_position = 0; // Track last printed position

// Movement primitives
void single_step(long pulse_delay, bool dir)
{
  digitalWrite(DIR_PIN, dir);
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(pulse_delay);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(pulse_delay);
  if (dir) fsm_ctx.position++;
  else fsm_ctx.position--;
  fsm_ctx.position = constrain(fsm_ctx.position, STEPPER_POSITION_MIN, STEPPER_POSITION_MAX);
  // Print position to serial when it changes
  if (fsm_ctx.position != last_printed_position) {
    Serial.print("Position: ");
    Serial.println(fsm_ctx.position);
    last_printed_position = fsm_ctx.position;
  }
}

// Non-blocking stepping state: use micros() to schedule steps so loop() stays responsive
unsigned long last_step_micros = 0;

// stepPeriodMicros is derived from PD (two pulse halves). We compute it each loop to reflect PD changes.
static inline unsigned long step_period_from_pd(long pulse_delay) {
  // single_step uses delayMicroseconds(pulse_delay) twice -> total ~2*pulse_delay
  unsigned long period = (unsigned long)(2UL * (unsigned long)max(1L, pulse_delay));
  return period;
}

// Log current limit switch status
void log_limit_status() {
  Serial.print("Limit switches: UP=");
  Serial.print(up_limit_tripped() ? "TRIPPED" : "OK");
  Serial.print(", DOWN=");
  Serial.println(down_limit_tripped() ? "TRIPPED" : "OK");
}

// Simulate limit switch state changes for testing
void update_limit_simulation() {
  if (!SIMULATE_LIMIT_SWITCHES) return;
  
  unsigned long now = millis();
  if (now - last_simulation_time >= SIMULATION_INTERVAL_MS) {
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
    send_message(simulated_up_limit ? CMD_UP_LIMIT_TRIP : CMD_UP_LIMIT_OK, STEPPER_PARAM_UNUSED);
    send_message(simulated_down_limit ? CMD_DOWN_LIMIT_TRIP : CMD_DOWN_LIMIT_OK, STEPPER_PARAM_UNUSED);
    
    sim_state = (sim_state + 1) % 4; // Cycle through 0-3
    last_simulation_time = now;
  }
}
volatile bool stop_flag = true;
bool direction = true; // true = up

// Helper to send Message to GUI (uses GUI_MAC defined earlier)
void send_message(CommandType cmd, int32_t param, uint8_t messageId)
{
  Message msg;
  msg.messageId = messageId;
  msg.command = cmd;
  msg.param = param;
  Serial.print("[SENT CMD] to GUI: id="); Serial.print(msg.messageId);
  Serial.print(" cmd="); Serial.print(commandToString(msg.command));
  Serial.print(" param="); Serial.println(msg.param);
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

// Move-to logic now handled in FSM

// Command handler now handled in FSM

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
  esp_now_register_recv_cb(on_data_recv);
  esp_now_register_send_cb(on_data_sent);

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
  Serial.println(fsm_ctx.position);
  
  // Load persisted pulse delay settings (if present)
  prefs.begin("stepper", false); // namespace "stepper"
  // Migration: prefer new short/stable keys (slow_pd, med_pd, fast_pd, moveto_pd)
  // If new key not present, fallback to old keys (slowPD, mediumPD, fastPD, moveToPD)
  long tmp;
  tmp = prefs.getLong("slow_pd", 0);
  if (tmp != 0) slow_pd = tmp;
  else {
    tmp = prefs.getLong("slowPD", 0);
    if (tmp != 0) { prefs.putLong("slow_pd", tmp); slow_pd = tmp; }
  }
  tmp = prefs.getLong("med_pd", 0);
  if (tmp != 0) med_pd = tmp;
  else {
    tmp = prefs.getLong("mediumPD", 0);
    if (tmp != 0) { prefs.putLong("med_pd", tmp); med_pd = tmp; }
  }
  tmp = prefs.getLong("fast_pd", 0);
  if (tmp != 0) fast_pd = tmp;
  else {
    tmp = prefs.getLong("fastPD", 0);
    if (tmp != 0) { prefs.putLong("fast_pd", tmp); fast_pd = tmp; }
  }
  tmp = prefs.getLong("moveto_pd", 0);
  if (tmp != 0) moveto_pd = tmp;
  else {
    tmp = prefs.getLong("moveToPD", 0);
    if (tmp != 0) { prefs.putLong("moveto_pd", tmp); moveto_pd = tmp; }
  }
  Serial.print("Loaded pulse delays: slow="); Serial.print(slow_pd);
  Serial.print(", medium="); Serial.print(med_pd);
  Serial.print(", fast="); Serial.print(fast_pd);
  Serial.print(", moveto="); Serial.println(moveto_pd);
  
  // Send reset command to GUI after controller startup
  delay(2000); // Wait for system to stabilize and GUI to be ready
  Serial.println("Sending reset command to GUI...");
  send_message(CMD_RESET, STEPPER_PARAM_UNUSED, 0);
  delay(500); // Give time for message to be sent
}

void loop()
{
  // Update limit switch simulation for testing
  update_limit_simulation();
  
  // Process queued messages
  Message msg;
  while (cb.pop(msg)) {
    Serial.print("[PROCESSING CMD] id="); Serial.print(msg.messageId);
    Serial.print(" cmd="); Serial.print(commandToString(msg.command));
    Serial.print(" param="); Serial.println(msg.param);
    fsm_handle_command(&fsm_ctx, msg);
  }

  // Non-blocking state machine stepping
  unsigned long now_micros = micros();
  unsigned long step_period = step_period_from_pd(pd);

  fsm_handle(&fsm_ctx, now_micros, step_period);

  // Small yield to avoid busy loop
  delay(1);
}

// (forward declarations moved earlier in the file)