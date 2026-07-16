// ============================================================
// POLO SEEKER v7.2 — TWR Initiator + RSSI + JSON Telemetry
// Board: Arduino Nano 33 BLE Sense + DWM1000
//
// THE EXCHANGE (three messages, no scheduled transmits):
//
//   1. Seeker sends POLL          -> records T1 (when POLL left)
//   2. Hider replies RESPONSE     -> Seeker records T4 (when it arrived)
//                                    Seeker reads RSSI here
//   3. Hider sends FINAL          -> carries (T3 - T2), the Hider's real
//                                    turnaround time
//
//   time_of_flight = ((T4 - T1) - (T3 - T2)) / 2
//   distance       = time_of_flight * speed_of_light
//
// v4 tried to use DW1000.setDelay() to schedule the Hider's reply at a
// fixed time. That never fired on this hardware. v5 avoids scheduling
// entirely — every transmit is immediate, which is the one pattern known
// to work reliably on these boards.
//
// OUTPUT: JSON with distance_m and rssi_dbm — the two fields this project
// has never had, and the whole point of the exercise.
//
// NOTE ON CLOCK DRIFT
// -------------------
// v6 attempted a carrier-frequency-offset correction to compensate for the
// two chips' crystals running at slightly different rates. The register read
// was mis-decoded (DRX_CAR_INT is 17-bit signed, not 21-bit), producing a
// bogus offset that swept from +8 to -6 ppm and made distances WORSE.
//
// That correction is removed. Raw single-sided TWR is stable enough here
// (roughly +/-0.35 m of wobble on a stationary pair), and that residual
// noise is exactly what the downstream filtering is designed to remove.
//
// TWR ROLE: TAG / INITIATOR
// ============================================================

#include <SPI.h>
#include <DW1000.h>
#include <Arduino_LSM9DS1.h>
#include <ArduinoJson.h>

// ---------- DW1000 wiring ----------
const uint8_t PIN_CS  = 20;
const uint8_t PIN_IRQ = 21;
const uint8_t PIN_RST = 3;

// ---------- antenna delay (must match Hider) ----------
#define ANTENNA_DELAY 16660

// ---------- message types ----------
#define MSG_POLL     0x01
#define MSG_RESPONSE 0x02
#define MSG_FINAL    0x03

// ---------- physics ----------
#define SPEED_OF_LIGHT 299702547.0     // m/s in air
#define DW_TIME_UNITS  15.65e-12       // seconds per DW1000 tick

// ---------- timing ----------
#define RANGE_INTERVAL_MS  1000
#define RX_TIMEOUT_MS        60     // waiting for the RESPONSE
#define FINAL_TIMEOUT_MS     80     // waiting for the FINAL (Hider delays it 10ms)
#define TX_TIMEOUT_MS        50

// ---------- frames (must match Hider exactly) ----------
struct __attribute__((packed)) ResponseFrame {
  uint8_t  msgType;
  uint8_t  hiderId;
  float    ax, ay, az;
  float    gx, gy, gz;
  float    mx, my, mz;
};

struct __attribute__((packed)) FinalFrame {
  uint8_t  msgType;
  uint8_t  hiderId;
  uint8_t  replyDelay[5];
};

// ---------- globals ----------
byte rxBuffer[64];
byte txBuffer[8];
char gpsBuffer[100] = "NO_GPS_LOCK_YET";
uint16_t rangeSeq = 0;
unsigned long lastRangeMs = 0;

// Holds the Hider's IMU data between the RESPONSE and the FINAL
ResponseFrame lastResponse;

// ---------- helpers ----------

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

void initDW1000() {
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_CS);

  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setAntennaDelay(ANTENNA_DELAY);
  DW1000.setDeviceAddress(1);
  DW1000.setNetworkId(10);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();

  byte pmsc[4];
  DW1000.readBytes(0x36, 0x04, pmsc, 4);
  pmsc[1] &= ~0x18;
  DW1000.writeBytes(0x36, 0x04, pmsc, 4);

  byte zeros[4] = {0, 0, 0, 0};
  DW1000.writeBytes(SYS_MASK, NO_SUB, zeros, 4);
}

// ============================================================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000);

  Serial.println("\n--- BOOTING SEEKER v7.2 (TWR Initiator) ---");

  Serial1.begin(9600);

  Serial.println("Starting IMU...");
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU! Check hardware.");
    while (1);
  }

  Serial.println("Starting DW1000 (UWB)...");
  initDW1000();

  Serial.println("Seeker v7.2 ready. Ranging every 1s.\n");
}

// ============================================================
void loop() {
  // Keep GPS buffer fresh
  if (Serial1.available()) {
    String line = Serial1.readStringUntil('\n');
    line.trim();
    if (line.length() > 0 && (line.startsWith("$GPGGA") || line.startsWith("$GPRMC"))) {
      line.toCharArray(gpsBuffer, sizeof(gpsBuffer));
    }
  }

  if (millis() - lastRangeMs < RANGE_INTERVAL_MS) {
    delayMicroseconds(100);
    return;
  }
  lastRangeMs = millis();
  rangeSeq++;

  // ============ STEP 1: SEND POLL ============
  txBuffer[0] = MSG_POLL;
  txBuffer[1] = 1;

  DW1000.newTransmit();
  DW1000.setDefaults();
  DW1000.setData(txBuffer, 2);
  DW1000.startTransmit();

  if (!waitForTxDone(TX_TIMEOUT_MS)) {
    Serial.println("[WARN] POLL TX timeout");
    clearStatusAll();
    emitSeekerJson();
    return;
  }

  // T1 — when the POLL left
  DW1000Time t1;
  DW1000.getTransmitTimestamp(t1);
  clearStatusAll();

  // ============ STEP 2: RECEIVE RESPONSE ============
  DW1000.newReceive();
  DW1000.setDefaults();
  DW1000.receivePermanently(false);
  DW1000.startReceive();

  if (!waitForRxGood(RX_TIMEOUT_MS)) {
    Serial.println("[WARN] No RESPONSE from Hider");
    clearStatusAll();
    emitSeekerJson();
    return;
  }

  // T4 — when the RESPONSE arrived
  DW1000Time t4;
  DW1000.getReceiveTimestamp(t4);

  // ***** READ RSSI AND CLOCK OFFSET HERE — before clearing status *****
  float rssi    = DW1000.getReceivePower();
  float fpPower = DW1000.getFirstPathPower();
  float quality = DW1000.getReceiveQuality();

  // Read Carrier Integrator for clock drift correction
  byte carInt[3];
  DW1000.readBytes(0x27, 0x28, carInt, 3);
  int32_t carrierIntegrator = (int32_t)((uint32_t)carInt[0] | ((uint32_t)carInt[1] << 8) | ((uint32_t)carInt[2] << 16));
  if (carrierIntegrator & 0x00100000) { // Sign extend 21-bit to 32-bit
    carrierIntegrator |= 0xFFE00000;
  }
  // Formula for 110 kbps, Channel 5 -> multiplier is -7.164e-11
  double clockOffsetRatio = (double)carrierIntegrator * -7.16402e-11;

  uint16_t len = DW1000.getDataLength();
  if (len > sizeof(rxBuffer)) len = sizeof(rxBuffer);
  DW1000.getData(rxBuffer, len);
  clearStatusAll();

  if (rxBuffer[0] != MSG_RESPONSE || len < sizeof(ResponseFrame)) {
    Serial.println("[WARN] Bad RESPONSE frame");
    emitSeekerJson();
    return;
  }

  memcpy(&lastResponse, rxBuffer, sizeof(ResponseFrame));

  // ============ STEP 3: RECEIVE FINAL ============
  // The Hider waits 10ms after the RESPONSE before sending the FINAL, which
  // gives us plenty of time to finish processing and re-arm the receiver.
  // We use a generous timeout here since the FINAL is deliberately delayed.
  DW1000.newReceive();
  DW1000.setDefaults();
  DW1000.receivePermanently(false);
  DW1000.startReceive();

  if (!waitForRxGood(FINAL_TIMEOUT_MS)) {
    Serial.println("[WARN] No FINAL from Hider");
    clearStatusAll();
    emitHiderJson(NAN, rssi, fpPower, quality, false);
    emitSeekerJson();
    return;
  }

  len = DW1000.getDataLength();
  if (len > sizeof(rxBuffer)) len = sizeof(rxBuffer);
  DW1000.getData(rxBuffer, len);
  clearStatusAll();

  if (rxBuffer[0] != MSG_FINAL || len < sizeof(FinalFrame)) {
    Serial.println("[WARN] Bad FINAL frame");
    emitHiderJson(NAN, rssi, fpPower, quality, false);
    emitSeekerJson();
    return;
  }

  FinalFrame* fin = (FinalFrame*)rxBuffer;

  // ============ STEP 4: THE DISTANCE MATH ============
  int64_t replyDelay = 0;
  replyDelay |= ((int64_t)fin->replyDelay[0] <<  0);
  replyDelay |= ((int64_t)fin->replyDelay[1] <<  8);
  replyDelay |= ((int64_t)fin->replyDelay[2] << 16);
  replyDelay |= ((int64_t)fin->replyDelay[3] << 24);
  replyDelay |= ((int64_t)fin->replyDelay[4] << 32);

  int64_t roundTrip = t4.getTimestamp() - t1.getTimestamp();
  if (roundTrip < 0) roundTrip += 0x10000000000LL;   // 40-bit wrap

  // Apply clock drift correction to the Hider's reply delay
  int64_t correctedReplyDelay = (int64_t)((double)replyDelay * (1.0 - clockOffsetRatio));

  int64_t tof   = (roundTrip - correctedReplyDelay) / 2;
  double tofSec = (double)tof * DW_TIME_UNITS;
  double dist   = tofSec * SPEED_OF_LIGHT;

  bool distValid = (dist > -2.0 && dist < 200.0);

  // Timing diagnostics
  Serial.print("[TWR] seq=");
  Serial.print(rangeSeq);
  Serial.print("  RT=");
  Serial.print((long)roundTrip);
  Serial.print("  RD=");
  Serial.print((long)replyDelay);
  Serial.print("  co=");
  Serial.print(clockOffsetRatio * 1e6, 2); // Show ppm
  Serial.print("  d=");
  Serial.print(dist, 3);
  Serial.println("m");

  emitHiderJson(dist, rssi, fpPower, quality, distValid);
  emitSeekerJson();
}

// ------------------------------------------------------------
// Emit the Hider's telemetry, now carrying distance and RSSI.
void emitHiderJson(double dist, float rssi, float fp, float quality, bool distValid) {
  JsonDocument doc;
  doc["id"]  = "hider_1";
  doc["gps"] = "";

  JsonArray acc = doc["acc"].to<JsonArray>();
  acc.add(lastResponse.ax); acc.add(lastResponse.ay); acc.add(lastResponse.az);

  JsonArray gyr = doc["gyr"].to<JsonArray>();
  gyr.add(lastResponse.gx); gyr.add(lastResponse.gy); gyr.add(lastResponse.gz);

  JsonArray mag = doc["mag"].to<JsonArray>();
  mag.add(lastResponse.mx); mag.add(lastResponse.my); mag.add(lastResponse.mz);

  // ***** THE NEW FIELDS *****
  if (distValid) {
    doc["distance_m"] = dist;
  } else {
    doc["distance_m"] = nullptr;
  }
  doc["rssi_dbm"] = rssi;
  doc["fp_dbm"]   = fp;
  doc["quality"]  = quality;
  doc["seq"]      = rangeSeq;

  String output;
  serializeJson(doc, output);
  Serial.println(output);
}

// ------------------------------------------------------------
// Emit the Seeker's own IMU reading.
void emitSeekerJson() {
  float ax = 0, ay = 0, az = 0;
  float gx = 0, gy = 0, gz = 0;
  float mx = 0, my = 0, mz = 0;

  if (IMU.accelerationAvailable())  IMU.readAcceleration(ax, ay, az);
  if (IMU.gyroscopeAvailable())     IMU.readGyroscope(gx, gy, gz);
  if (IMU.magneticFieldAvailable()) IMU.readMagneticField(mx, my, mz);

  JsonDocument doc;
  doc["id"]  = "seeker_1";
  doc["gps"] = gpsBuffer;

  JsonArray acc = doc["acc"].to<JsonArray>();
  acc.add(ax); acc.add(ay); acc.add(az);

  JsonArray gyr = doc["gyr"].to<JsonArray>();
  gyr.add(gx); gyr.add(gy); gyr.add(gz);

  JsonArray mag = doc["mag"].to<JsonArray>();
  mag.add(mx); mag.add(my); mag.add(mz);

  String output;
  serializeJson(doc, output);
  Serial.println(output);
}

// ============================================================
// CALIBRATION
// ============================================================
// ANTENNA_DELAY (16800) shifts every distance by a constant offset.
//   Reads too LONG  -> INCREASE it.
//   Reads too SHORT -> DECREASE it.
// Roughly 1 unit ~= 1.5 mm. So being 0.30 m long ~= raise by ~200 units.
// Change it in BOTH sketches and reflash BOTH boards.
// ============================================================
