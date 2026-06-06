// ============================================================
// DWM1000 Antenna Delay Calibration — RESPONDER
// Board: Arduino Nano 33 BLE Sense Lite + DWM1000 shield
//
// Purpose: Respond to initiator's POLL messages during
//          antenna delay calibration. Logs its own RX
//          diagnostics and environmental data over serial.
//
// *** BLE is NOT initialized — zero radio interference ***
// *** All output is CSV over USB Serial at 115200 baud ***
//
// Procedure:
//   1. Flash this sketch on Device B (the responder)
//   2. Flash cal_initiator.ino on Device A
//   3. Power up both, open Serial Monitor on each
//   4. Responder runs automatically — no commands needed
//   5. Optionally type 'status' or 'env' for info
//
// Serial Commands:
//   delay XXXXX   — set antenna delay register (0–65535)
//   status        — print current settings
//   env           — read environmental sensors once
//   reset         — reset exchange counter
// ============================================================

#include <SPI.h>
#include <DW1000.h>
#include <Arduino_LPS22HB.h>

// ---------- DW1000 wiring ----------
const uint8_t PIN_CS  = 10;
const uint8_t PIN_IRQ = 2;
const uint8_t PIN_RST = 3;

// ---------- TWR message types ----------
#define MSG_POLL     0x01
#define MSG_RESPONSE 0x02

// ---------- antenna delay ----------
uint16_t antennaDelay = 0;   // ZERO for calibration

#define DW_TIME_UNITS  15.65e-12

// ---------- state ----------
uint16_t exchangeCount = 0;
bool     sensorsAvail  = false;

float envTemp     = 0.0;
float envPressure = 0.0;

// ---------- buffers ----------
byte rxBuffer[20];
byte txBuffer[20];

// ============================================================
// Low-level helpers
// ============================================================

static inline void clearStatusAll() {
  byte clear[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  DW1000.writeBytes(SYS_STATUS, NO_SUB, clear, 5);
}

void startReceiver() {
  DW1000.newReceive();
  DW1000.receivePermanently(true);
  DW1000.startReceive();
}

void setAntennaDelay(uint16_t delay16) {
  byte d[2];
  d[0] = delay16 & 0xFF;
  d[1] = (delay16 >> 8) & 0xFF;
  DW1000.writeBytes(0x18, 0x00, d, 2);     // TX_ANTD
  DW1000.writeBytes(0x2E, 0x1804, d, 2);   // LDE_RXANTD
}

uint16_t readAntennaDelay() {
  byte d[2];
  DW1000.readBytes(0x18, 0x00, d, 2);
  return d[0] | (d[1] << 8);
}

void readEnvironment() {
  if (!sensorsAvail) return;
  envTemp     = BARO.readTemperature();
  envPressure = BARO.readPressure() * 10.0f;   // kPa → hPa
}

void printStatus() {
  Serial.println(F("\n--- Responder Status ---"));
  Serial.print(F("  Antenna delay reg : ")); Serial.println(antennaDelay);
  Serial.print(F("  Antenna delay (ns): ")); Serial.println((double)antennaDelay * DW_TIME_UNITS * 1e9, 3);
  Serial.print(F("  Exchanges served  : ")); Serial.println(exchangeCount);
  Serial.print(F("  Sensors available : ")); Serial.println(sensorsAvail ? "YES" : "NO");
  if (sensorsAvail) {
    readEnvironment();
    Serial.print(F("  Temperature (C)   : ")); Serial.println(envTemp, 2);
    Serial.print(F("  Pressure (hPa)    : ")); Serial.println(envPressure, 1);
  }
  Serial.println(F("------------------------\n"));
}

void handleSerial() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toLowerCase();

  if (cmd.startsWith("delay ")) {
    long d = cmd.substring(6).toInt();
    if (d >= 0 && d <= 65535) {
      antennaDelay = (uint16_t)d;
      setAntennaDelay(antennaDelay);
      Serial.print(F("Antenna delay set to: ")); Serial.print(antennaDelay);
      Serial.print(F(" (")); Serial.print((double)antennaDelay * DW_TIME_UNITS * 1e9, 3);
      Serial.println(F(" ns)"));
      // Restart receiver with new delay
      startReceiver();
    }
  } else if (cmd == "status") {
    printStatus();
  } else if (cmd == "env") {
    readEnvironment();
    Serial.print(F("Temp: ")); Serial.print(envTemp, 2); Serial.print(F(" C  "));
    Serial.print(F("Pressure: ")); Serial.print(envPressure, 1); Serial.println(F(" hPa"));
  } else if (cmd == "reset") {
    exchangeCount = 0;
    Serial.println(F("Exchange counter reset."));
  } else {
    Serial.println(F("Commands: delay XXXXX, status, env, reset"));
  }
}

// ============================================================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000);

  Serial.println(F("============================================"));
  Serial.println(F(" DWM1000 Calibration — RESPONDER"));
  Serial.println(F(" NO BLE — Serial output only"));
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
  DW1000.setDeviceAddress(1);   // responder address
  DW1000.setNetworkId(10);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();

  // Set antenna delay to ZERO
  setAntennaDelay(antennaDelay);

  // Disable auto-sleep
  byte pmsc[4];
  DW1000.readBytes(0x36, 0x04, pmsc, 4);
  pmsc[1] &= ~0x18;
  DW1000.writeBytes(0x36, 0x04, pmsc, 4);

  // Disable DW1000 interrupts
  byte zeros[4] = {0, 0, 0, 0};
  DW1000.writeBytes(SYS_MASK, NO_SUB, zeros, 4);

  uint16_t readBack = readAntennaDelay();
  Serial.print(F("Antenna delay register: ")); Serial.print(readBack);
  Serial.print(F(" (")); Serial.print((double)readBack * DW_TIME_UNITS * 1e9, 3);
  Serial.println(F(" ns)"));

  Serial.println();
  printStatus();

  // Print CSV header for responder's own log
  Serial.println(F("# Responder log — one line per exchange"));
  Serial.println(F("exchange,millis,rx_power_dBm,fp_power_dBm,quality,reply_delay_ticks,temp_C,pressure_hPa"));

  startReceiver();
  Serial.println(F("\nListening for POLL messages...\n"));
}

// ============================================================
void loop() {
  handleSerial();

  // Check DW1000 RX status
  byte status[5];
  DW1000.readBytes(SYS_STATUS, NO_SUB, status, 5);

  bool dataReady = (status[1] & 0x20);
  bool goodCRC   = (status[1] & 0x40);

  if (dataReady && goodCRC) {
    // T2 — RX timestamp of the poll
    DW1000Time t2;
    DW1000.getReceiveTimestamp(t2);

    // Signal diagnostics of the received poll
    float rxPower = DW1000.getReceivePower();
    float fpPower = DW1000.getFirstPathPower();
    float quality = DW1000.getReceiveQuality();

    // Read payload
    uint16_t len = DW1000.getDataLength();
    if (len > sizeof(rxBuffer)) len = sizeof(rxBuffer);
    DW1000.getData(rxBuffer, len);

    clearStatusAll();

    if (rxBuffer[0] == MSG_POLL) {
      exchangeCount++;

      // ---- Send RESPONSE #1 (immediate) ----
      txBuffer[0] = MSG_RESPONSE;
      txBuffer[1] = 0x01;   // responder ID

      DW1000.newTransmit();
      DW1000.setData(txBuffer, 2);
      DW1000.startTransmit();

      // Wait for TX done
      unsigned long start = millis();
      while ((millis() - start) < 50) {
        DW1000.readBytes(SYS_STATUS, NO_SUB, status, 5);
        if (status[0] & 0x80) break;
        delayMicroseconds(50);
      }

      // T3 — TX timestamp of response
      DW1000Time t3;
      DW1000.getTransmitTimestamp(t3);

      int64_t replyDelay = t3.getTimestamp() - t2.getTimestamp();

      clearStatusAll();

      // ---- Send RESPONSE #2 (carries reply delay) ----
      txBuffer[0] = MSG_RESPONSE;
      txBuffer[1] = (replyDelay >>  0) & 0xFF;
      txBuffer[2] = (replyDelay >>  8) & 0xFF;
      txBuffer[3] = (replyDelay >> 16) & 0xFF;
      txBuffer[4] = (replyDelay >> 24) & 0xFF;
      txBuffer[5] = (replyDelay >> 32) & 0xFF;

      DW1000.newTransmit();
      DW1000.setData(txBuffer, 6);
      DW1000.startTransmit();

      start = millis();
      while ((millis() - start) < 50) {
        DW1000.readBytes(SYS_STATUS, NO_SUB, status, 5);
        if (status[0] & 0x80) break;
        delayMicroseconds(50);
      }
      clearStatusAll();

      // Read environment every 25th exchange
      if (sensorsAvail && (exchangeCount % 25 == 0 || exchangeCount == 1)) {
        readEnvironment();
      }

      // CSV log for responder
      Serial.print(exchangeCount);         Serial.print(',');
      Serial.print(millis());              Serial.print(',');
      Serial.print(rxPower, 2);            Serial.print(',');
      Serial.print(fpPower, 2);            Serial.print(',');
      Serial.print(quality, 2);            Serial.print(',');
      Serial.print((long)replyDelay);      Serial.print(',');
      Serial.print(envTemp, 2);            Serial.print(',');
      Serial.println(envPressure, 1);
    }

    startReceiver();

  } else if (dataReady) {
    // Bad CRC
    clearStatusAll();
    startReceiver();
  }

  delayMicroseconds(100);
}
