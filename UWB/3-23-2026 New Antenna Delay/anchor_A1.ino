// ============================================================
// UWB Anchor (A1) — SS-TWR Responder + BLE Telemetry
// Board: Arduino Nano 33 BLE Sense Lite + DWM1000 shield
//
// Listens for POLL messages, sends a single delayed response
// containing both the timing reference and the 40-bit reply
// delay (T3-T2), and broadcasts RX diagnostics over BLE.
//
// BLE device name format: A1, A2, A3 …  (set DEVICE_ID below)
//
// Revision: Single-response protocol using DW1000 delayed TX.
//           Antenna delay = 16800.
// ============================================================

#include <SPI.h>
#include <DW1000.h>
#include <ArduinoBLE.h>

// ---------- identity ----------
#define DEVICE_ID   1                     // 1 → "A1", 2 → "A2", etc.
#define DEVICE_NAME "A1"                  // must match DEVICE_ID

// ---------- DW1000 wiring ----------
const uint8_t PIN_CS  = 10;
const uint8_t PIN_IRQ = 2;
const uint8_t PIN_RST = 3;

// ---------- antenna delay ----------
#define ANTENNA_DELAY 16800               // calibrated value (0-65535)

// ---------- TWR message types ----------
#define MSG_POLL     0x01
#define MSG_RESPONSE 0x02

// ---------- fixed reply delay ----------
// The anchor schedules its response at a fixed delay after
// receiving the poll.  3000 µs / 15.65e-6 µs ≈ 191 693 290 ticks.
// This must be long enough for the MCU to prepare the TX buffer
// but short enough to avoid DW1000 counter wrap issues.
#define FIXED_REPLY_DW  191693290LL

// ---------- watchdog ----------
// If no complete POLL→RESPONSE exchange happens within this window,
// the DWM1000 is soft-reset and the receiver restarted.
#define WATCHDOG_MS  5000UL

// ---------- BLE UUIDs ----------
#define UWB_SERVICE_UUID   "19b10010-e8f2-537e-4f6c-d104768a1214"
#define ANCHOR_CHAR_UUID   "19b10012-e8f2-537e-4f6c-d104768a1214"

// ---------- BLE data frame ----------
// 33 bytes total — well within the ~244-byte ATT payload the
// nRF52840 negotiates with a modern BLE central (e.g. Bleak).
//
// Field layout (little-endian, packed):
//   tag_id         uint8    1   which tag sent the poll
//   seq            uint16   2   exchange sequence
//   rx_power       float    4   RX power of received poll (dBm)
//   fp_power       float    4   first-path power of poll (dBm)
//   quality        float    4   preamble quality (CIR/noise²)
//   std_noise      uint16   2   RX_FQUAL[0:1]  noise std-dev (raw)
//   fp_ampl1       uint16   2   RX_TIME[7:8]   1st-path amplitude (raw)
//   fp_ampl2       uint16   2   RX_FQUAL[2:3]  2nd-path amplitude (raw)
//   fp_ampl3       uint16   2   RX_FQUAL[4:5]  3rd-path amplitude (raw)
//   cir_power      uint16   2   RX_FQUAL[6:7]  CIR power (raw)
//   rxpacc         uint16   2   RX_FINFO[20:31] preamble accumulations
//   reply_delay_lo int32    4   (T3-T2) lower 32 DW ticks
//   reply_delay_hi uint8    1   (T3-T2) upper  8 DW ticks
//   flags          uint8    1   bit0 = exchange_ok
//                          ---
//                          33 bytes
struct __attribute__((packed)) AnchorFrame {
  uint8_t  tag_id;
  uint16_t seq;
  float    rx_power;
  float    fp_power;
  float    quality;
  uint16_t std_noise;
  uint16_t fp_ampl1;
  uint16_t fp_ampl2;
  uint16_t fp_ampl3;
  uint16_t cir_power;
  uint16_t rxpacc;
  int32_t  reply_delay_lo;
  uint8_t  reply_delay_hi;
  uint8_t  flags;
};

// ---------- RX diagnostics ----------
struct RawDiag {
  float    rx_power;
  float    fp_power;
  float    quality;
  uint16_t std_noise;
  uint16_t fp_ampl1;
  uint16_t fp_ampl2;
  uint16_t fp_ampl3;
  uint16_t cir_power;
  uint16_t rxpacc;
};

// ---------- globals ----------
byte rxBuffer[20];
byte txBuffer[20];
uint16_t rangeSeq  = 0;
uint32_t lastGoodMs = 0;   // millis() of last completed exchange (watchdog)

BLEService        uwbService(UWB_SERVICE_UUID);
BLECharacteristic anchorChar(ANCHOR_CHAR_UUID, BLERead | BLENotify, sizeof(AnchorFrame));

// ---------- helpers ----------

static inline void clearStatusAll() {
  byte clear[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  DW1000.writeBytes(SYS_STATUS, NO_SUB, clear, 5);
}

void startReceiver() {
  DW1000.newReceive();
  DW1000.setDefaults();
  DW1000.receivePermanently(true);
  DW1000.startReceive();
}

// Pulse RST, re-run full DW1000 init, restart receiver.
// Call whenever the chip appears stuck (TX timeout or watchdog expiry).
void dwmSoftReset() {
  Serial.println(F("[RST] DWM soft-reset…"));

  // Assert RST low → chip resets
  pinMode(PIN_RST, OUTPUT);
  digitalWrite(PIN_RST, LOW);
  delay(2);
  // Release RST (tri-state; board pull-up takes it high)
  pinMode(PIN_RST, INPUT);
  delay(10);   // wait for CLKPLL to lock (~5 ms typical)

  // Re-initialise exactly as in setup()
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_CS);

  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setAntennaDelay(ANTENNA_DELAY);
  DW1000.setDeviceAddress(DEVICE_ID);
  DW1000.setNetworkId(10);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();

  // Disable auto-sleep
  byte pmsc[4];
  DW1000.readBytes(0x36, 0x04, pmsc, 4);
  pmsc[1] &= ~0x18;
  DW1000.writeBytes(0x36, 0x04, pmsc, 4);

  // Disable interrupts
  byte zeros[4] = {0, 0, 0, 0};
  DW1000.writeBytes(SYS_MASK, NO_SUB, zeros, 4);

  startReceiver();
  Serial.println(F("[RST] Done. Listening…"));
}

RawDiag readRxDiagnostics() {
  RawDiag d;

  d.rx_power = DW1000.getReceivePower();
  d.fp_power = DW1000.getFirstPathPower();
  d.quality  = DW1000.getReceiveQuality();

  // RX_FQUAL (0x12, 8 bytes):
  //   [0:1] STD_NOISE  [2:3] FP_AMPL2  [4:5] FP_AMPL3  [6:7] CIR_PWR
  byte fqual[8];
  DW1000.readBytes(0x12, NO_SUB, fqual, 8);
  d.std_noise = (uint16_t)fqual[0] | ((uint16_t)fqual[1] << 8);
  d.fp_ampl2  = (uint16_t)fqual[2] | ((uint16_t)fqual[3] << 8);
  d.fp_ampl3  = (uint16_t)fqual[4] | ((uint16_t)fqual[5] << 8);
  d.cir_power = (uint16_t)fqual[6] | ((uint16_t)fqual[7] << 8);

  // FP_AMPL1 from RX_TIME (0x15), bytes 7-8
  byte rxtime[9];
  DW1000.readBytes(0x15, NO_SUB, rxtime, 9);
  d.fp_ampl1 = (uint16_t)rxtime[7] | ((uint16_t)rxtime[8] << 8);

  // RXPACC from RX_FINFO (0x10), bits 20-31
  byte rxfinfo[4];
  DW1000.readBytes(0x10, NO_SUB, rxfinfo, 4);
  uint32_t fi = (uint32_t)rxfinfo[0]        |
                ((uint32_t)rxfinfo[1] <<  8) |
                ((uint32_t)rxfinfo[2] << 16) |
                ((uint32_t)rxfinfo[3] << 24);
  d.rxpacc = (uint16_t)((fi >> 20) & 0xFFF);

  return d;
}

// ============================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("=== UWB Anchor + BLE  [" DEVICE_NAME "] ==="));

  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_CS);

  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setAntennaDelay(ANTENNA_DELAY);
  DW1000.setDeviceAddress(DEVICE_ID);
  DW1000.setNetworkId(10);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();

  // Disable auto-sleep
  byte pmsc[4];
  DW1000.readBytes(0x36, 0x04, pmsc, 4);
  pmsc[1] &= ~0x18;
  DW1000.writeBytes(0x36, 0x04, pmsc, 4);

  // Disable DW1000 interrupts
  byte zeros[4] = {0, 0, 0, 0};
  DW1000.writeBytes(SYS_MASK, NO_SUB, zeros, 4);

  if (!BLE.begin()) {
    Serial.println(F("BLE init failed!"));
    while (1) delay(1000);
  }

  BLE.setDeviceName(DEVICE_NAME);
  BLE.setLocalName(DEVICE_NAME);
  BLE.setAdvertisedService(uwbService);
  uwbService.addCharacteristic(anchorChar);
  BLE.addService(uwbService);

  AnchorFrame z = {};
  anchorChar.writeValue((uint8_t*)&z, sizeof(AnchorFrame));

  BLE.advertise();
  Serial.println(F("BLE advertising. Listening for POLLs…\n"));
  startReceiver();
  lastGoodMs = millis();
}

// ============================================================
void loop() {
  BLE.poll();

  // ---- Background watchdog ----
  if ((uint32_t)(millis() - lastGoodMs) > WATCHDOG_MS) {
    Serial.print(F("[WDT] No exchange for "));
    Serial.print(WATCHDOG_MS);
    Serial.println(F(" ms — resetting DWM"));
    dwmSoftReset();
    lastGoodMs = millis();
    return;
  }

  byte status[5];
  DW1000.readBytes(SYS_STATUS, NO_SUB, status, 5);

  bool dataReady = (status[1] & 0x20);
  bool goodCRC   = (status[1] & 0x40);

  if (dataReady && goodCRC) {
    // ---- T2: RX timestamp of the poll ----
    DW1000Time t2;
    DW1000.getReceiveTimestamp(t2);

    // Read ALL diagnostics before clearing status
    RawDiag diag = readRxDiagnostics();

    uint16_t len = DW1000.getDataLength();
    if (len > sizeof(rxBuffer)) len = sizeof(rxBuffer);
    DW1000.getData(rxBuffer, len);

    clearStatusAll();

    if (rxBuffer[0] == MSG_POLL) {
      rangeSeq++;

      uint8_t tagId = 0;
      if (len >= 2) tagId = rxBuffer[1];

      // ---- Build response with fixed delayed TX ----
      // Schedule TX at T2 + FIXED_REPLY_DW so the DW1000 transmits
      // at a deterministic time.  The actual TX timestamp (T3) is
      // read back after transmission for the BLE telemetry frame.
      int64_t scheduledTx = t2.getTimestamp() + FIXED_REPLY_DW;

      // Pack response: [0]=type [1]=anchorId [2..6]=replyDelay (5 bytes LE)
      // We send the FIXED_REPLY_DW value; the tag uses it for ToF calc.
      // The actual T3 will match closely but the tag needs the value
      // that was used to *schedule* the TX for the math to be consistent.
      txBuffer[0] = MSG_RESPONSE;
      txBuffer[1] = DEVICE_ID;
      txBuffer[2] = (FIXED_REPLY_DW >>  0) & 0xFF;
      txBuffer[3] = (FIXED_REPLY_DW >>  8) & 0xFF;
      txBuffer[4] = (FIXED_REPLY_DW >> 16) & 0xFF;
      txBuffer[5] = (FIXED_REPLY_DW >> 24) & 0xFF;
      txBuffer[6] = (FIXED_REPLY_DW >> 32) & 0xFF;

      DW1000Time txTime;
      txTime.setTimestamp(scheduledTx);

      DW1000.newTransmit();
      DW1000.setDefaults();
      DW1000.setData(txBuffer, 7);
      DW1000.setDelay(txTime);
      DW1000.startTransmit();

      // Wait for TX complete
      unsigned long start = millis();
      bool txok = false;
      while ((millis() - start) < 50) {
        DW1000.readBytes(SYS_STATUS, NO_SUB, status, 5);
        if (status[0] & 0x80) { txok = true; break; }
        delayMicroseconds(50);
      }
      if (!txok) {
        Serial.println(F("[ERR] TX timeout — resetting DWM"));
        clearStatusAll();
        dwmSoftReset();
        lastGoodMs = millis();
        return;
      }

      // Read actual TX timestamp for BLE telemetry
      DW1000Time t3;
      DW1000.getTransmitTimestamp(t3);
      int64_t actualReply = t3.getTimestamp() - t2.getTimestamp();

      clearStatusAll();

      // ---- Build & send BLE frame ----
      AnchorFrame frame;
      frame.tag_id         = tagId;
      frame.seq            = rangeSeq;
      frame.rx_power       = diag.rx_power;
      frame.fp_power       = diag.fp_power;
      frame.quality        = diag.quality;
      frame.std_noise      = diag.std_noise;
      frame.fp_ampl1       = diag.fp_ampl1;
      frame.fp_ampl2       = diag.fp_ampl2;
      frame.fp_ampl3       = diag.fp_ampl3;
      frame.cir_power      = diag.cir_power;
      frame.rxpacc         = diag.rxpacc;
      frame.reply_delay_lo = (int32_t)(actualReply & 0xFFFFFFFFLL);
      frame.reply_delay_hi = (uint8_t)((actualReply >> 32) & 0xFF);
      frame.flags          = 0x01;

      anchorChar.writeValue((uint8_t*)&frame, sizeof(AnchorFrame));

      // Exchange fully completed — pet the watchdog
      lastGoodMs = millis();

      // Serial debug
      Serial.print(F("A")); Serial.print(DEVICE_ID);
      Serial.print(F("←T")); Serial.print(tagId);
      Serial.print(F(" #")); Serial.print(rangeSeq);
      Serial.print(F("  RX="));   Serial.print(diag.rx_power, 1);
      Serial.print(F("  FP="));   Serial.print(diag.fp_power, 1);
      Serial.print(F("  Q="));    Serial.print(diag.quality, 1);
      Serial.print(F("  SN="));   Serial.print(diag.std_noise);
      Serial.print(F("  A1="));   Serial.print(diag.fp_ampl1);
      Serial.print(F("  A2="));   Serial.print(diag.fp_ampl2);
      Serial.print(F("  A3="));   Serial.print(diag.fp_ampl3);
      Serial.print(F("  PACC=")); Serial.print(diag.rxpacc);
      Serial.print(F("  RD="));   Serial.print((long)actualReply);
      Serial.println();
    }

    startReceiver();

  } else if (dataReady) {
    // Frame received but bad CRC — clear and keep listening
    clearStatusAll();
    startReceiver();
  }

  delayMicroseconds(100);
}
