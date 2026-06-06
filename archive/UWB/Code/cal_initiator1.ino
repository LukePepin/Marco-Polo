// ============================================================
// DWM1000 Antenna Delay Calibration — INITIATOR
// Board: Arduino Nano 33 BLE Sense Lite + DWM1000 shield
//
// Purpose: Collect raw TWR data at a known distance for
//          per-module antenna delay calibration.
//
// *** BLE is NOT initialized — zero radio interference ***
// *** All output is CSV over USB Serial at 115200 baud ***
//
// Procedure:
//   1. Flash this sketch on Device A (the initiator)
//   2. Flash cal_responder.ino on Device B
//   3. Place 1.0 m apart, antennas facing each other
//   4. Open Serial Monitor on Device A at 115200
//   5. Type 'start' to begin collecting samples
//   6. Copy CSV output → paste into spreadsheet or use
//      cal_postprocess.py to compute antenna delay
//
// Serial Commands:
//   start         — begin collecting (default 500 samples)
//   start N       — collect N samples (e.g. 'start 1000')
//   stop          — abort collection
//   delay XXXXX   — set antenna delay register (0–65535)
//   distance X.XX — set known reference distance in meters
//   status        — print current settings
//   env           — read environmental sensors once
// ============================================================

#include <SPI.h>
#include <DW1000.h>
#include <Arduino_LPS22HB.h>   // Temperature + Barometric Pressure

// ---------- DW1000 wiring ----------
const uint8_t PIN_CS  = 10;
const uint8_t PIN_IRQ = 2;    // not used (polled)
const uint8_t PIN_RST = 3;

// ---------- TWR message types ----------
#define MSG_POLL     0x01
#define MSG_RESPONSE 0x02

// ---------- physics ----------
#define SPEED_OF_LIGHT 299702547.0        // m/s in air (sea level)
#define DW_TIME_UNITS  15.65e-12          // ~15.65 ps per tick

// ---------- timing ----------
#define RANGE_INTERVAL_MS   50            // fast: 20 Hz during cal
#define RX_TIMEOUT_MS       80
#define TX_TIMEOUT_MS       50

// ---------- antenna delay ----------
// DW1000 16-bit register, each LSB = 15.65 ps
// Default ~16384 from setDefaults(), typical calibrated ~32900
uint16_t antennaDelay = 0;                // START AT ZERO for calibration

// ---------- calibration state ----------
float    knownDistance   = 1.00;          // meters — set to your measured distance
uint16_t targetSamples  = 500;
uint16_t sampleCount    = 0;
bool     collecting      = false;
bool     sensorsAvail    = false;

// ---------- environmental sensor cache ----------
float envTemp     = 0.0;
float envPressure = 0.0;

// ---------- buffers ----------
byte rxBuffer[20];
byte txBuffer[20];

// ============================================================
// Low-level DW1000 helpers (polled, no interrupts)
// ============================================================

static inline void clearStatusAll() {
  byte clear[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  DW1000.writeBytes(SYS_STATUS, NO_SUB, clear, 5);
}

static inline bool waitForTxDone(uint16_t timeoutMs) {
  byte status[5];
  unsigned long start = millis();
  while ((millis() - start) < timeoutMs) {
    DW1000.readBytes(SYS_STATUS, NO_SUB, status, 5);
    if (status[0] & 0x80) return true;   // TXFRS
    delayMicroseconds(50);
  }
  return false;
}

static inline bool waitForRxGood(uint16_t timeoutMs) {
  byte status[5];
  unsigned long start = millis();
  while ((millis() - start) < timeoutMs) {
    DW1000.readBytes(SYS_STATUS, NO_SUB, status, 5);
    if (status[1] & 0x40) return true;   // RXFCG
    delayMicroseconds(100);
  }
  return false;
}

// ---------- Set DW1000 antenna delay register ----------
void setAntennaDelay(uint16_t delay16) {
  // TX_ANTD register: 0x18, 2 bytes
  byte d[2];
  d[0] = delay16 & 0xFF;
  d[1] = (delay16 >> 8) & 0xFF;
  DW1000.writeBytes(0x18, 0x00, d, 2);   // TX_ANTD

  // LDE_RXANTD: register 0x2E, sub 0x1804, 2 bytes
  DW1000.writeBytes(0x2E, 0x1804, d, 2); // RX antenna delay
}

uint16_t readAntennaDelay() {
  byte d[2];
  DW1000.readBytes(0x18, 0x00, d, 2);
  return d[0] | (d[1] << 8);
}

// ---------- Read environmental sensors ----------
void readEnvironment() {
  if (!sensorsAvail) return;
  envTemp     = BARO.readTemperature();
  envPressure = BARO.readPressure() * 10.0f;   // kPa → hPa
}

// ---------- Print current config ----------
void printStatus() {
  Serial.println(F("\n--- Calibration Status ---"));
  Serial.print(F("  Antenna delay reg : ")); Serial.println(antennaDelay);
  Serial.print(F("  Antenna delay (ns): ")); Serial.println((double)antennaDelay * DW_TIME_UNITS * 1e9, 3);
  Serial.print(F("  Known distance (m): ")); Serial.println(knownDistance, 3);
  Serial.print(F("  Target samples    : ")); Serial.println(targetSamples);
  Serial.print(F("  Collecting        : ")); Serial.println(collecting ? "YES" : "NO");
  Serial.print(F("  Sensors available : ")); Serial.println(sensorsAvail ? "YES" : "NO");
  if (sensorsAvail) {
    readEnvironment();
    Serial.print(F("  Temperature (C)   : ")); Serial.println(envTemp, 2);
    Serial.print(F("  Pressure (hPa)    : ")); Serial.println(envPressure, 1);
  }
  Serial.println(F("--------------------------\n"));
}

// ---------- Parse serial commands ----------
void handleSerial() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "start" || cmd.startsWith("start ")) {
    if (cmd.length() > 6) {
      int n = cmd.substring(6).toInt();
      if (n > 0 && n <= 10000) targetSamples = n;
    }
    sampleCount = 0;
    collecting = true;

    // Read environment at start
    readEnvironment();

    // Print CSV header
    Serial.println(F("\n# DWM1000 Antenna Delay Calibration — Initiator"));
    Serial.print(F("# Known distance: ")); Serial.print(knownDistance, 4); Serial.println(F(" m"));
    Serial.print(F("# Antenna delay register: ")); Serial.println(antennaDelay);
    Serial.print(F("# Antenna delay (ns): ")); Serial.println((double)antennaDelay * DW_TIME_UNITS * 1e9, 3);
    Serial.print(F("# Target samples: ")); Serial.println(targetSamples);
    Serial.print(F("# Start temp (C): ")); Serial.println(envTemp, 2);
    Serial.print(F("# Start pressure (hPa): ")); Serial.println(envPressure, 1);
    Serial.print(F("# Range interval: ")); Serial.print(RANGE_INTERVAL_MS); Serial.println(F(" ms"));
    Serial.println(F("#"));
    Serial.println(F("sample,millis,distance_m,round_trip_ticks,reply_delay_ticks,tof_ticks,rx_power_dBm,fp_power_dBm,quality,temp_C,pressure_hPa"));

  } else if (cmd == "stop") {
    collecting = false;
    Serial.println(F("\n# Collection stopped by user."));

  } else if (cmd.startsWith("delay ")) {
    long d = cmd.substring(6).toInt();
    if (d >= 0 && d <= 65535) {
      antennaDelay = (uint16_t)d;
      setAntennaDelay(antennaDelay);
      Serial.print(F("Antenna delay set to: ")); Serial.print(antennaDelay);
      Serial.print(F(" (")); Serial.print((double)antennaDelay * DW_TIME_UNITS * 1e9, 3);
      Serial.println(F(" ns)"));
    } else {
      Serial.println(F("ERROR: delay must be 0–65535"));
    }

  } else if (cmd.startsWith("distance ")) {
    float d = cmd.substring(9).toFloat();
    if (d > 0.0 && d < 1000.0) {
      knownDistance = d;
      Serial.print(F("Known distance set to: ")); Serial.print(knownDistance, 4); Serial.println(F(" m"));
    } else {
      Serial.println(F("ERROR: distance must be > 0 and < 1000 m"));
    }

  } else if (cmd == "status") {
    printStatus();

  } else if (cmd == "env") {
    readEnvironment();
    Serial.print(F("Temp: ")); Serial.print(envTemp, 2); Serial.print(F(" C  "));
    Serial.print(F("Pressure: ")); Serial.print(envPressure, 1); Serial.println(F(" hPa"));

  } else {
    Serial.println(F("Commands: start [N], stop, delay XXXXX, distance X.XX, status, env"));
  }
}

// ============================================================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000);  // wait up to 5s for USB serial

  Serial.println(F("============================================"));
  Serial.println(F(" DWM1000 Calibration — INITIATOR"));
  Serial.println(F(" NO BLE — Serial CSV output only"));
  Serial.println(F("============================================"));

  // ----- Environmental sensor (LPS22HB: temp + pressure) -----
  sensorsAvail = true;
  if (!BARO.begin()) {
    Serial.println(F("WARN: LPS22HB (temp/pressure) not found"));
    sensorsAvail = false;
  }
  if (sensorsAvail) {
    Serial.println(F("LPS22HB sensor: OK"));
  }

  // ----- DW1000 init -----
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_CS);

  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(101);    // initiator address
  DW1000.setNetworkId(10);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();

  // Set antenna delay to ZERO for raw measurement
  setAntennaDelay(antennaDelay);

  // Disable auto-sleep
  byte pmsc[4];
  DW1000.readBytes(0x36, 0x04, pmsc, 4);
  pmsc[1] &= ~0x18;
  DW1000.writeBytes(0x36, 0x04, pmsc, 4);

  // Disable DW1000 interrupts (polled)
  byte zeros[4] = {0, 0, 0, 0};
  DW1000.writeBytes(SYS_MASK, NO_SUB, zeros, 4);

  // Verify antenna delay register
  uint16_t readBack = readAntennaDelay();
  Serial.print(F("Antenna delay register: ")); Serial.print(readBack);
  Serial.print(F(" (")); Serial.print((double)readBack * DW_TIME_UNITS * 1e9, 3);
  Serial.println(F(" ns)"));

  Serial.println();
  printStatus();
  Serial.println(F("Type 'start' to begin collecting, 'status' for info."));
  Serial.println(F("Set distance first with 'distance 1.00' if not 1.0 m.\n"));
}

// ============================================================
void loop() {
  handleSerial();

  if (!collecting) return;

  // ========== SEND POLL ==========
  txBuffer[0] = MSG_POLL;
  txBuffer[1] = 0x01;   // initiator ID

  DW1000.newTransmit();
  DW1000.setData(txBuffer, 2);
  DW1000.startTransmit();

  if (!waitForTxDone(TX_TIMEOUT_MS)) {
    clearStatusAll();
    delay(RANGE_INTERVAL_MS);
    return;
  }

  // T1 — poll TX timestamp
  DW1000Time t1;
  DW1000.getTransmitTimestamp(t1);
  clearStatusAll();

  // ========== WAIT FOR RESPONSE #1 (get T4) ==========
  DW1000.newReceive();
  DW1000.receivePermanently(false);
  DW1000.startReceive();

  if (!waitForRxGood(RX_TIMEOUT_MS)) {
    clearStatusAll();
    delay(RANGE_INTERVAL_MS);
    return;
  }

  // T4 — response receive timestamp
  DW1000Time t4;
  DW1000.getReceiveTimestamp(t4);

  // Signal diagnostics
  float rxPower = DW1000.getReceivePower();
  float fpPower = DW1000.getFirstPathPower();
  float quality = DW1000.getReceiveQuality();

  uint16_t len1 = DW1000.getDataLength();
  if (len1 > sizeof(rxBuffer)) len1 = sizeof(rxBuffer);
  DW1000.getData(rxBuffer, len1);
  clearStatusAll();

  // ========== WAIT FOR RESPONSE #2 (reply delay) ==========
  DW1000.newReceive();
  DW1000.receivePermanently(false);
  DW1000.startReceive();

  if (!waitForRxGood(RX_TIMEOUT_MS)) {
    clearStatusAll();
    delay(RANGE_INTERVAL_MS);
    return;
  }

  uint16_t len2 = DW1000.getDataLength();
  if (len2 > sizeof(rxBuffer)) len2 = sizeof(rxBuffer);
  DW1000.getData(rxBuffer, len2);
  clearStatusAll();

  if (rxBuffer[0] != MSG_RESPONSE || len2 < 6) {
    delay(RANGE_INTERVAL_MS);
    return;
  }

  // Extract reply delay (T3 - T2) — 40-bit LE
  int64_t replyDelay = 0;
  replyDelay |= ((int64_t)rxBuffer[1] <<  0);
  replyDelay |= ((int64_t)rxBuffer[2] <<  8);
  replyDelay |= ((int64_t)rxBuffer[3] << 16);
  replyDelay |= ((int64_t)rxBuffer[4] << 24);
  replyDelay |= ((int64_t)rxBuffer[5] << 32);

  // ========== COMPUTE ==========
  int64_t roundTrip = t4.getTimestamp() - t1.getTimestamp();
  if (roundTrip < 0) roundTrip += 0x10000000000LL;   // 40-bit wrap

  int64_t tofTicks = (roundTrip - replyDelay) / 2;
  double tofSec    = (double)tofTicks * DW_TIME_UNITS;
  double distance  = tofSec * SPEED_OF_LIGHT;

  sampleCount++;

  // Read environment every 25th sample (not every sample — I2C is slow)
  if (sensorsAvail && (sampleCount % 25 == 0 || sampleCount == 1)) {
    readEnvironment();
  }

  // ========== CSV OUTPUT ==========
  // sample,millis,distance_m,round_trip_ticks,reply_delay_ticks,tof_ticks,
  //   rx_power_dBm,fp_power_dBm,quality,temp_C,pressure_hPa
  Serial.print(sampleCount);          Serial.print(',');
  Serial.print(millis());             Serial.print(',');
  Serial.print(distance, 4);          Serial.print(',');
  Serial.print((long)roundTrip);      Serial.print(',');
  Serial.print((long)replyDelay);     Serial.print(',');
  Serial.print((long)tofTicks);       Serial.print(',');
  Serial.print(rxPower, 2);           Serial.print(',');
  Serial.print(fpPower, 2);           Serial.print(',');
  Serial.print(quality, 2);           Serial.print(',');
  Serial.print(envTemp, 2);           Serial.print(',');
  Serial.println(envPressure, 1);

  // ========== CHECK COMPLETION ==========
  if (sampleCount >= targetSamples) {
    collecting = false;

    // Final environment reading
    readEnvironment();

    Serial.println(F("#"));
    Serial.print(F("# Collection complete: ")); Serial.print(sampleCount); Serial.println(F(" samples"));
    Serial.print(F("# End temp (C): ")); Serial.println(envTemp, 2);
    Serial.print(F("# End pressure (hPa): ")); Serial.println(envPressure, 1);
    Serial.println(F("#"));
    Serial.println(F("# Paste CSV into cal_postprocess.py or spreadsheet."));
    Serial.println(F("# To re-run: type 'start' or 'start N'"));
  }

  delay(RANGE_INTERVAL_MS);
}
