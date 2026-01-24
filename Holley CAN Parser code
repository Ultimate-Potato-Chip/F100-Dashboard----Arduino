/*
 * 1960-1966 F100 Digital Gauge Cluster
 * ESP32-S3 Nano + Holley Sniper 2 CAN Bus Integration
 * 
 * Hardware:
 * - ESP32-S3 Nano (8MB PSRAM)
 * - SN65HVD230 CAN Transceiver
 * - 5x Round IPS LCD Displays
 * - Oil Pressure Sender (0-5V analog)
 * - T56 VSS Hall Sensor
 * 
 * CAN Messages from Sniper 2:
 * - RPM: 0x9E005000
 * - Coolant Temp: 0x9E069000
 * - Battery Voltage: 0x9E06D000
 * - Speed: 0x9E1C1000
 */

#include <Arduino.h>
#include "driver/twai.h"  // ESP32 CAN (TWAI) driver

// ============================================================================
// CAN BUS CONFIGURATION
// ============================================================================

// CAN GPIO Pins
#define CAN_TX_PIN GPIO_NUM_4
#define CAN_RX_PIN GPIO_NUM_5

// Sniper 2 CAN Message IDs (Extended 29-bit)
#define CAN_ID_RPM         0x9E005000
#define CAN_ID_COOLANT     0x9E069000
#define CAN_ID_BATTERY     0x9E06D000
#define CAN_ID_SPEED       0x9E1C1000

// Optional diagnostic messages
#define CAN_ID_IAC         0x9E059000  // Idle Air Control Position
#define CAN_ID_MAP         0x9E05D000  // Manifold Pressure
#define CAN_ID_MAT         0x9E065800  // Manifold Air Temp
#define CAN_ID_AFR         0x9E019000  // Air/Fuel Ratio

// ============================================================================
// SENSOR INPUT CONFIGURATION
// ============================================================================

#define OIL_PRESSURE_PIN   34  // Analog input for oil pressure sender
#define VSS_PIN            35  // Hall effect VSS pulse input
#define FUEL_LEVEL_PIN     36  // Analog input for fuel sender

// ============================================================================
// GLOBAL VARIABLES - Gauge Data
// ============================================================================

struct GaugeData {
  // Primary gauges
  float rpm;              // 0-6000 RPM (reasonable for 300 I6)
  float coolantTemp;      // -40 to 260°F
  float batteryVoltage;   // 0-20V
  float speed;            // 0-120 MPH
  float oilPressure;      // 0-100 PSI
  float fuelLevel;        // 0-100%
  
  // Diagnostic data
  float iac;              // 0-100%
  float map;              // 0-999 kPa
  float mat;              // -40 to 260°F
  float afr;              // 2-20 A/F ratio
  
  // Status flags
  bool engineRunning;
  bool lowOilPressure;
  bool lowBattery;
  bool highCoolantTemp;
  
  // Timestamp for data freshness
  unsigned long lastUpdate;
};

GaugeData gauges = {0};

// VSS pulse counting for speed calculation
volatile unsigned long vssCount = 0;
volatile unsigned long lastVssTime = 0;
unsigned long lastSpeedCalc = 0;

// ============================================================================
// CAN BUS INITIALIZATION
// ============================================================================

void setupCAN() {
  // Configure CAN timing for 1000 kbit/s (Sniper 2 standard)
  twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_1MBITS();
  
  // Configure CAN filter to accept all extended IDs
  twai_filter_config_t filter_config = {
    .acceptance_code = 0,
    .acceptance_mask = 0xFFFFF800,
    .single_filter = false
  };
  
  // Configure CAN general settings
  twai_general_config_t general_config = {
    .mode = TWAI_MODE_NORMAL,
    .tx_io = CAN_TX_PIN,
    .rx_io = CAN_RX_PIN,
    .clkout_io = TWAI_IO_UNUSED,
    .bus_off_io = TWAI_IO_UNUSED,
    .tx_queue_len = 5,
    .rx_queue_len = 10,
    .alerts_enabled = TWAI_ALERT_NONE,
    .clkout_divider = 0,
    .intr_flags = ESP_INTR_FLAG_LEVEL1
  };
  
  // Install CAN driver
  if (twai_driver_install(&general_config, &timing_config, &filter_config) == ESP_OK) {
    Serial.println("CAN Driver installed");
  } else {
    Serial.println("Failed to install CAN driver");
    return;
  }
  
  // Start CAN driver
  if (twai_start() == ESP_OK) {
    Serial.println("CAN Driver started");
  } else {
    Serial.println("Failed to start CAN driver");
  }
}

// ============================================================================
// CAN MESSAGE PARSING
// ============================================================================

void parseCANMessage(twai_message_t *message) {
  // Check if extended ID (29-bit)
  if (!(message->extd)) return;
  
  uint32_t id = message->identifier;
  uint8_t *data = message->data;
  
  // Parse based on message ID
  switch (id) {
    case CAN_ID_RPM: {
      // RPM is 32-bit signed integer in bytes 0-3
      int32_t rpm_raw;
      memcpy(&rpm_raw, &data[0], 4);
      gauges.rpm = (float)rpm_raw;
      gauges.engineRunning = (gauges.rpm > 200);  // Engine running threshold
      break;
    }
    
    case CAN_ID_COOLANT: {
      // Coolant temp is 32-bit signed integer (°F) in bytes 0-3
      int32_t temp_raw;
      memcpy(&temp_raw, &data[0], 4);
      gauges.coolantTemp = (float)temp_raw;
      gauges.highCoolantTemp = (gauges.coolantTemp > 220);  // Warning threshold
      break;
    }
    
    case CAN_ID_BATTERY: {
      // Battery voltage is 32-bit signed integer in bytes 0-3
      int32_t voltage_raw;
      memcpy(&voltage_raw, &data[0], 4);
      gauges.batteryVoltage = (float)voltage_raw;
      gauges.lowBattery = (gauges.batteryVoltage < 12.5 && gauges.engineRunning);
      break;
    }
    
    case CAN_ID_SPEED: {
      // Speed is 32-bit signed integer (MPH) in bytes 0-3
      int32_t speed_raw;
      memcpy(&speed_raw, &data[0], 4);
      gauges.speed = (float)speed_raw;
      break;
    }
    
    // Optional diagnostic messages
    case CAN_ID_IAC: {
      int32_t iac_raw;
      memcpy(&iac_raw, &data[0], 4);
      gauges.iac = (float)iac_raw;
      break;
    }
    
    case CAN_ID_MAP: {
      int32_t map_raw;
      memcpy(&map_raw, &data[0], 4);
      gauges.map = (float)map_raw;
      break;
    }
    
    case CAN_ID_MAT: {
      int32_t mat_raw;
      memcpy(&mat_raw, &data[0], 4);
      gauges.mat = (float)mat_raw;
      break;
    }
    
    case CAN_ID_AFR: {
      int32_t afr_raw;
      memcpy(&afr_raw, &data[0], 4);
      gauges.afr = (float)afr_raw;
      break;
    }
  }
  
  gauges.lastUpdate = millis();
}

// ============================================================================
// CAN MESSAGE RECEPTION (Call in loop)
// ============================================================================

void updateCANData() {
  twai_message_t message;
  
  // Check for received messages (non-blocking)
  while (twai_receive(&message, 0) == ESP_OK) {
    parseCANMessage(&message);
  }
}

bool canDataValid() {
  return (millis() - gauges.lastUpdate) < 250;
}

// ============================================================================
// VSS (Vehicle Speed Sensor) INTERRUPT HANDLER
// ============================================================================

void IRAM_ATTR vssInterrupt() {
  vssCount++;
  lastVssTime = micros();
}

void setupVSS() {
  pinMode(VSS_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(VSS_PIN), vssInterrupt, RISING);
}

void updateVSSSpeed() {
  // Calculate speed every 100ms
  unsigned long now = millis();
  if (now - lastSpeedCalc < 100) return;
  
  unsigned long pulses = vssCount;
  vssCount = 0;  // Reset counter
  
  // T56 VSS typical: 17 pulses per driveshaft revolution
  // Rear axle ratio (e.g., 3.73:1)
  // Tire circumference (e.g., 82 inches = 6.83 feet)
  // 
  // Speed calculation:
  // pulses_per_second = pulses / (interval / 1000)
  // revs_per_second = pulses_per_second / 17
  // wheel_revs_per_second = revs_per_second / 3.73
  // feet_per_second = wheel_revs_per_second * 6.83
  // mph = feet_per_second * 3600 / 5280
  //
  // Simplified: MPH = pulses * K, where K depends on your setup
  // You'll need to calibrate this constant!
  
  float interval = (now - lastSpeedCalc) / 1000.0;  // seconds
  float pulsesPerSecond = pulses / interval;
  
  // Example calculation (CALIBRATE THIS FOR YOUR TRUCK!)
  const float VSS_PULSES_PER_MILE = 8000;  // Adjust based on testing
  float milesPerHour = (pulsesPerSecond * 3600.0) / VSS_PULSES_PER_MILE;
  
  // Override CAN speed with VSS speed if available
  if (pulsesPerSecond > 0) {
    gauges.speed = milesPerHour;
  }
  
  lastSpeedCalc = now;
}

// ============================================================================
// ANALOG SENSOR READING
// ============================================================================

void updateOilPressure() {
  // Read analog voltage from oil pressure sender
  int rawValue = analogRead(OIL_PRESSURE_PIN);
  float voltage = (rawValue / 4095.0) * 3.3;  // ESP32 is 12-bit ADC, 3.3V max
  
  // Most low-voltage oil pressure senders are 0.5V (0 PSI) to 4.5V (100 PSI)
  // Linear interpolation:
  gauges.oilPressure = ((voltage - 0.5) / 4.0) * 100.0;
  
  // Clamp to valid range
  if (gauges.oilPressure < 0) gauges.oilPressure = 0;
  if (gauges.oilPressure > 100) gauges.oilPressure = 100;
  
  // Warning threshold: Low oil pressure when engine running
  gauges.lowOilPressure = (gauges.oilPressure < 10 && gauges.engineRunning);
}

void updateFuelLevel() {
  // Read fuel sender resistance via voltage divider
  int rawValue = analogRead(FUEL_LEVEL_PIN);
  float voltage = (rawValue / 4095.0) * 3.3;
  
  // Voltage divider: 3.3V ---[330Ω]---[Fuel Sender]--- GND
  //                                 |
  //                            ADC Input
  // 
  // Calculate sender resistance:
  // V = 3.3 * (R_sender / (330 + R_sender))
  // R_sender = (330 * V) / (3.3 - V)
  
  const float R_KNOWN = 330.0;  // Known resistor in voltage divider
  float senderResistance = (R_KNOWN * voltage) / (3.3 - voltage);
  
  // Ford fuel sender typical: 73Ω (full) to 10Ω (empty)
  // Map resistance to percentage
  const float R_FULL = 73.0;
  const float R_EMPTY = 10.0;
  
  gauges.fuelLevel = ((senderResistance - R_EMPTY) / (R_FULL - R_EMPTY)) * 100.0;
  
  // Clamp to valid range
  if (gauges.fuelLevel < 0) gauges.fuelLevel = 0;
  if (gauges.fuelLevel > 100) gauges.fuelLevel = 100;
}

// ============================================================================
// SETUP & MAIN LOOP
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println("F100 Gauge Cluster Starting...");
  
  // Initialize CAN bus
  setupCAN();
  
  // Initialize VSS input
  setupVSS();
  
  // Initialize analog inputs
  analogReadResolution(12);  // 12-bit ADC
  pinMode(OIL_PRESSURE_PIN, INPUT);
  pinMode(FUEL_LEVEL_PIN, INPUT);
  
  Serial.println("Initialization complete!");
}

void loop() {
  // Update all sensor data
  updateCANData();          // Read CAN messages from Sniper 2
  if (!canDataValid()) {    // Check for stale CANData
  Serial.println("CAN data stale");}
  updateVSSSpeed();         // Calculate speed from VSS pulses
  updateOilPressure();      // Read oil pressure sender
  updateFuelLevel();        // Read fuel level sender
  
  // Debug output (remove once displays are working)
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    Serial.println("=== Gauge Data ===");
    Serial.printf("RPM: %.0f | Coolant: %.0f°F | Battery: %.1fV\n", 
                  gauges.rpm, gauges.coolantTemp, gauges.batteryVoltage);
    Serial.printf("Speed: %.0f MPH | Oil: %.0f PSI | Fuel: %.0f%%\n",
                  gauges.speed, gauges.oilPressure, gauges.fuelLevel);
    Serial.println();
    lastPrint = millis();
  }
  
  // TODO: Update LVGL displays here
  // updateDisplays();
  
  delay(10);  // Small delay for stability
}
