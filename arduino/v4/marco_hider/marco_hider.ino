// ============================================================
// MARCO HIDER v7.2 — TWR Responder (NO delayed transmit)
// Board: Arduino Nano 33 BLE Sense + DWM1000
//
// WHY v5 EXISTS
// -------------
// v4 used DW1000.setDelay() to schedule the response at a fixed future
// time. On THIS hardware (CS=20/A6, IRQ=21/A7) the scheduled transmit
// never fires — TXFRS never sets and every exchange times out.
//
// Meanwhile the v3 firmware transmitted IMMEDIATELY (no setDelay) and
// worked reliably for months. So: immediate TX works, delayed TX does not.
//
// v5 therefore uses a THREE-MESSAGE exchange that needs no scheduling:
//
//   1. Seeker sends POLL           (Seeker records T1 = when it left)
//   2. Hider receives POLL         (Hider records T2 = when it arrived)
//      Hider sends RESPONSE now    (Hider records T3 = when it actually left)
//   3. Hider sends FINAL           (carries T3-T2, the real reply delay)
//
// The Hider cannot put T3 inside the RESPONSE itself — it does not know T3
// until after the RESPONSE has been sent. Hence the separate FINAL message.
//
// The Seeker records T4 = when RESPONSE arrived, then computes:
//   time_of_flight = ((T4 - T1) - (T3 - T2)) / 2
//   distance       = time_of_flight * speed_of_light
//
// TWR ROLE: ANCHOR / RESPONDER
// ============================================================

#include <SPI.h>
#include <DW1000.h>
#include <Arduino_LSM9DS1.h>

// ---------- DW1000 wiring (current hardware) ----------
const uint8_t PIN_CS  = 20;
const uint8_t PIN_IRQ = 21;
const uint8_t PIN_RST = 3;

// ---------- antenna delay ----------
// Must match the Seeker. Shifts all distances by a constant offset.
// Calibrate against a tape measure once ranging works.
#define ANTENNA_DELAY 16660

// ---------- message types ----------
#define MSG_POLL     0x01
#define MSG_RESPONSE 0x02
#define MSG_FINAL    0x03

// ---------- watchdog ----------
#define WATCHDOG_MS  10000UL

// ---------- IMU cache ----------
// Refreshed in the main loop, NEVER inside the response path.
// I2C sensor reads take milliseconds; the response path must stay fast.
#define IMU_REFRESH_MS  50

// ---------- RESPONSE frame ----------
// Sent immediately on receiving a POLL. Carries the IMU data.
// Size: 2 + 36 = 38 bytes.
struct __attribute__((packed)) ResponseFrame {
  uint8_t  msgType;      // MSG_RESPONSE
  uint8_t  hiderId;
  float    ax, ay, az;
  float    gx, gy, gz;
  float    mx, my, mz;
};

// ---------- FINAL frame ----------
// Sent right after the RESPONSE. Carries the ACTUAL measured reply delay
// (T3 - T2), which the Hider only knows after the RESPONSE has gone out.
// Size: 2 + 5 = 7 bytes.
struct __attribute__((packed)) FinalFrame {
  uint8_t  msgType;         // MSG_FINAL
  uint8_t  hiderId;
  uint8_t  replyDelay[5];   // 40-bit (T3 - T2), little-endian
};

// ---------- globals ----------
byte rxBuffer[32];
ResponseFrame response;
FinalFrame    finalMsg;

uint32_t lastGoodMs = 0;
uint32_t lastImuMs  = 0;
uint16_t exchangeCount = 0;

// Cached IMU values
float c_ax = 0, c_ay = 0, c_az = 0;
float c_gx = 0, c_gy = 0, c_gz = 0;
float c_mx = 0, c_my = 0, c_mz = 0;

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

// Transmit immediately (NO setDelay) and poll for completion.
// This is the pattern v3 used successfully — it is known to work here.
bool transmitNow(byte* data, uint16_t len, uint16_t timeoutMs) {
  DW1000.newTransmit();
  DW1000.setDefaults();
  DW1000.setData(data, len);
  DW1000.startTransmit();

  byte status[5];
  unsigned long start = millis();
  while ((millis() - start) < timeoutMs) {
    DW1000.readBytes(SYS_STATUS, NO_SUB, status, 5);
    if (status[0] & 0x80) return true;   // TXFRS
    delayMicroseconds(50);
  }
  return false;
}

void initDW1000() {
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_CS);

  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setAntennaDelay(ANTENNA_DELAY);
  DW1000.setDeviceAddress(2);
  DW1000.setNetworkId(10);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();

  // Disable auto-sleep
  byte pmsc[4];
  DW1000.readBytes(0x36, 0x04, pmsc, 4);
  pmsc[1] &= ~0x18;
  DW1000.writeBytes(0x36, 0x04, pmsc, 4);

  // Disable interrupts — poll instead (Mbed OS crashes on SPI-in-ISR)
  byte zeros[4] = {0, 0, 0, 0};
  DW1000.writeBytes(SYS_MASK, NO_SUB, zeros, 4);
}

void dwmSoftReset() {
  Serial.println("[RST] DWM soft-reset...");
  pinMode(PIN_RST, OUTPUT);
  digitalWrite(PIN_RST, LOW);
  delay(2);
  pinMode(PIN_RST, INPUT);
  delay(10);
  initDW1000();
  startReceiver();
  Serial.println("[RST] Done. Listening...");
}

void refreshImuCache() {
  float ax, ay, az, gx, gy, gz, mx, my, mz;
  if (IMU.accelerationAvailable())   { IMU.readAcceleration(ax, ay, az); c_ax=ax; c_ay=ay; c_az=az; }
  if (IMU.gyroscopeAvailable())      { IMU.readGyroscope(gx, gy, gz);    c_gx=gx; c_gy=gy; c_gz=gz; }
  if (IMU.magneticFieldAvailable())  { IMU.readMagneticField(mx, my, mz); c_mx=mx; c_my=my; c_mz=mz; }
}

// ============================================================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000);

  Serial.println("\n--- BOOTING HIDER v7.2 (TWR Responder, immediate TX) ---");

  Serial.println("Starting IMU...");
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU! Check hardware.");
    while (1);
  }

  refreshImuCache();
  delay(50);
  refreshImuCache();

  Serial.println("Starting DW1000 (UWB)...");
  initDW1000();

  Serial.println("Starting receiver...");
  startReceiver();

  lastGoodMs = millis();
  lastImuMs  = millis();
  Serial.println("Hider v7.2 ready. Listening for POLLs...\n");
}

// ============================================================
void loop() {
  // Keep the IMU cache fresh, outside the response path
  if (millis() - lastImuMs >= IMU_REFRESH_MS) {
    refreshImuCache();
    lastImuMs = millis();
  }

  // Watchdog
  if ((uint32_t)(millis() - lastGoodMs) > WATCHDOG_MS) {
    Serial.println("[WDT] No exchange in 10s — resetting DWM");
    dwmSoftReset();
    lastGoodMs = millis();
    return;
  }

  // Poll the status register for an incoming packet
  byte status[5];
  DW1000.readBytes(SYS_STATUS, NO_SUB, status, 5);

  bool dataReady = (status[1] & 0x20);   // RXDFR
  bool goodCRC   = (status[1] & 0x40);   // RXFCG

  if (dataReady && goodCRC) {

    // ---- T2: when the POLL arrived ----
    DW1000Time t2;
    DW1000.getReceiveTimestamp(t2);

    uint16_t len = DW1000.getDataLength();
    if (len > sizeof(rxBuffer)) len = sizeof(rxBuffer);
    DW1000.getData(rxBuffer, len);
    clearStatusAll();

    if (rxBuffer[0] == MSG_POLL) {
      exchangeCount++;

      // ---- Send RESPONSE immediately (no scheduling) ----
      response.msgType = MSG_RESPONSE;
      response.hiderId = 1;
      response.ax = c_ax; response.ay = c_ay; response.az = c_az;
      response.gx = c_gx; response.gy = c_gy; response.gz = c_gz;
      response.mx = c_mx; response.my = c_my; response.mz = c_mz;

      if (!transmitNow((byte*)&response, sizeof(ResponseFrame), 50)) {
        Serial.println("[ERR] RESPONSE TX failed");
        clearStatusAll();
        startReceiver();
        return;
      }

      // ---- T3: when the RESPONSE actually left ----
      // We only know this AFTER transmitting. That is why a separate
      // FINAL message is needed to carry it.
      DW1000Time t3;
      DW1000.getTransmitTimestamp(t3);
      clearStatusAll();

      // ---- The real, measured reply delay ----
      int64_t replyDelay = t3.getTimestamp() - t2.getTimestamp();
      if (replyDelay < 0) replyDelay += 0x10000000000LL;   // 40-bit wrap

      // ---- Give the Seeker time to re-arm its receiver ----
      // After the RESPONSE, the Seeker has real work to do: read RSSI, read
      // diagnostics, pull the frame, clear status, tear down the receiver and
      // rebuild it. Only THEN is it listening again. If we fire the FINAL
      // before it is ready, the message is simply gone.
      //
      // v6 used 2ms and still dropped most FINALs. 10ms is generous and
      // costs nothing: the FINAL carries no timing-critical data, just a
      // number. It can arrive whenever the Seeker is ready to hear it.
      delay(10);

      // ---- Send FINAL carrying that delay ----
      finalMsg.msgType = MSG_FINAL;
      finalMsg.hiderId = 1;
      finalMsg.replyDelay[0] = (replyDelay >>  0) & 0xFF;
      finalMsg.replyDelay[1] = (replyDelay >>  8) & 0xFF;
      finalMsg.replyDelay[2] = (replyDelay >> 16) & 0xFF;
      finalMsg.replyDelay[3] = (replyDelay >> 24) & 0xFF;
      finalMsg.replyDelay[4] = (replyDelay >> 32) & 0xFF;

      if (!transmitNow((byte*)&finalMsg, sizeof(FinalFrame), 50)) {
        Serial.println("[ERR] FINAL TX failed");
        clearStatusAll();
        startReceiver();
        return;
      }

      clearStatusAll();
      lastGoodMs = millis();

      Serial.print("EXCH #");
      Serial.print(exchangeCount);
      Serial.print("  replyDelay=");
      Serial.print((long)replyDelay);
      Serial.print("  acc=[");
      Serial.print(c_ax, 3); Serial.print(",");
      Serial.print(c_ay, 3); Serial.print(",");
      Serial.print(c_az, 3); Serial.println("]");
    }

    startReceiver();

  } else if (dataReady) {
    clearStatusAll();
    startReceiver();
  }

  delayMicroseconds(100);
}
