// =====================================================
// ESP32 Dual VL53L0X + OLED (FAST)  |  Serial @ 460800
// Prints EXACT format every loop:
//   "<d1>a<d2>b\n"  OR  "NaNb\n"
// Where "Na" means sensor1 invalid, "Nb" means sensor2 invalid.
//
// Goal: keep loop fast, but keep valid readings up to ~400mm.
// Fixes vs your version:
//  1) LONG_RANGE sensor preset (better >200mm reliability)
//  2) Less-strict validity check (don’t drop usable >200mm readings)
//  3) Keep timing budget low (fast) + allow I2C 400k
//  4) Same alternating read strategy (1 sensor per loop), print every loop
// =====================================================

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_VL53L0X.h>
#include <Wire.h>

// ------------------ PINS ------------------
// ToF I2C bus (Wire)
#define SDA_PIN 18
#define SCL_PIN 23
#define XSHUT_1 14
#define XSHUT_2 27

// OLED I2C bus (Wire1)
#define OLED_ADDR 0x3C
#define OLED_SDA 21
#define OLED_SCL 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// ------------------ SERIAL ------------------
static const uint32_t BAUD = 460800;

// ------------------ SPEED / TUNING ------------------
// ToF I2C clock (400k is fine on ESP32 if wiring is decent)
static const uint32_t TOF_I2C_HZ = 400000;
// OLED I2C clock
static const uint32_t OLED_I2C_HZ = 400000;

// Keep FAST but still good long-ish range
// Try 20000 first. If you want even faster, try 15000 or 12000.
// Lower budget = faster but more dropouts.
static const uint32_t TIMING_BUDGET_US = 20000;

// OLED FPS cap
static const uint32_t OLED_FPS = 20;
static const uint32_t OLED_INTERVAL_MS = 1000 / OLED_FPS;

// For OLED visualization mapping (doesn't affect serial)
static const float IN_MIN_MM = 0.0f;
static const float IN_MAX_MM = 400.0f;
static const float AMP_MIN_PX = 4.0f;
static const float AMP_MAX_PX = 28.0f;
static const float FREQ_MIN_CYCLES = 0.25f;
static const float FREQ_MAX_CYCLES = 3.0f;

// ------------------ OBJECTS ------------------
Adafruit_VL53L0X lox1;
Adafruit_VL53L0X lox2;

TwoWire WireOLED = TwoWire(1);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &WireOLED, -1);

VL53L0X_RangingMeasurementData_t m1;
VL53L0X_RangingMeasurementData_t m2;

// ------------------ STATE ------------------
uint16_t last1 = 0, last2 = 0;
bool valid1 = false, valid2 = false;

// ------------------ UTILS ------------------
static inline float clampFloat(float v, float lo, float hi) {
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

static inline float mapFloat(float x, float in_min, float in_max, float out_min,
                             float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Accept more statuses than "RangeStatus==0" so >200mm doesn't become Na/Nb
// VL53L0X has various non-zero statuses that can still provide usable distance.
// We only hard-reject true out-of-range + sentinel values.
static inline bool readingUsable(const VL53L0X_RangingMeasurementData_t &m) {
  // Library sentinel for invalid/out-of-range
  if (m.RangeMilliMeter == 8191)
    return false;

  // RangeStatus == 4 is commonly "out of range"
  if (m.RangeStatus == 4)
    return false;

  // Basic sanity
  if (m.RangeMilliMeter == 0)
    return false;
  if (m.RangeMilliMeter > 1200)
    return false; // protect against garbage

  return true;
}

// Fast decimal conversion (no snprintf)
static inline int u16_to_dec(char *dst, uint16_t v) {
  char tmp[5];
  int n = 0;
  do {
    tmp[n++] = char('0' + (v % 10));
    v /= 10;
  } while (v && n < 5);
  for (int i = 0; i < n; i++)
    dst[i] = tmp[n - 1 - i];
  return n;
}

// Print EXACT format: "<d1>a<d2>b\n" or "NaNb\n"
static inline void printLine(bool v1, uint16_t d1, bool v2, uint16_t d2) {
  char line[24];
  int n = 0;

  if (v1) {
    n += u16_to_dec(line + n, d1);
    line[n++] = 'a';
  } else {
    line[n++] = 'N';
    line[n++] = 'a';
  }

  if (v2) {
    n += u16_to_dec(line + n, d2);
    line[n++] = 'b';
  } else {
    line[n++] = 'N';
    line[n++] = 'b';
  }

  line[n++] = '\n';
  Serial.write((const uint8_t *)line, n);
}

// ------------------ SETUP ------------------
void setup() {
  Serial.begin(BAUD);
  delay(300);

#if defined(ARDUINO_ARCH_ESP32)
  // Helps avoid Serial blocking if host stalls
  Serial.setTxBufferSize(4096);
#endif

  // ---- ToF I2C bus ----
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(TOF_I2C_HZ);

  pinMode(XSHUT_1, OUTPUT);
  pinMode(XSHUT_2, OUTPUT);

  // Reset both sensors
  digitalWrite(XSHUT_1, LOW);
  digitalWrite(XSHUT_2, LOW);
  delay(20);

  // Bring up sensor 1 (0x30)
  digitalWrite(XSHUT_1, HIGH);
  delay(80);
  if (!lox1.begin(0x30, &Wire)) {
    Serial.println("Sensor #1 not detected @0x30");
    while (1)
      delay(10);
  }

  // Bring up sensor 2 (0x31)
  digitalWrite(XSHUT_2, HIGH);
  delay(80);
  if (!lox2.begin(0x31, &Wire)) {
    Serial.println("Sensor #2 not detected @0x31");
    while (1)
      delay(10);
  }

  // LONG RANGE preset improves stability past ~200mm
  lox1.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_LONG_RANGE);
  lox2.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_LONG_RANGE);

  // Keep budget low (FAST) but not too low
  lox1.setMeasurementTimingBudgetMicroSeconds(TIMING_BUDGET_US);
  lox2.setMeasurementTimingBudgetMicroSeconds(TIMING_BUDGET_US);

  // ---- OLED bus ----
  WireOLED.begin(OLED_SDA, OLED_SCL);
  WireOLED.setClock(OLED_I2C_HZ);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 allocation failed");
    while (1)
      delay(100);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("VL53L0X x2");
  display.println("LONG_RANGE");
  display.println("FAST serial");
  display.display();
  delay(150);
}

// ------------------ LOOP ------------------
void loop() {
  static bool readSensor1 = true;
  static uint32_t lastOledMs = 0;

  // ---- Read ONE sensor per loop (alternating) ----
  if (readSensor1) {
    lox1.rangingTest(&m1, false);
    valid1 = readingUsable(m1);
    if (valid1)
      last1 = (uint16_t)m1.RangeMilliMeter;
  } else {
    lox2.rangingTest(&m2, false);
    valid2 = readingUsable(m2);
    if (valid2)
      last2 = (uint16_t)m2.RangeMilliMeter;
  }
  readSensor1 = !readSensor1;

  // ---- Print EVERY loop ----
  // NOTE: This prints the latest known value for each sensor.
  // If one sensor hasn't updated this loop, it still prints its last value.
  printLine(valid1, last1, valid2, last2);

  // ---- OLED at fixed FPS ----
  uint32_t now = millis();
  if (now - lastOledMs >= OLED_INTERVAL_MS) {
    lastOledMs = now;

    float d1 = valid1 ? (float)last1 : IN_MIN_MM;
    float d2 = valid2 ? (float)last2 : (IN_MIN_MM + IN_MAX_MM) * 0.5f;

    d1 = clampFloat(d1, IN_MIN_MM, IN_MAX_MM);
    d2 = clampFloat(d2, IN_MIN_MM, IN_MAX_MM);

    float amplitudePx =
        mapFloat(d1, IN_MIN_MM, IN_MAX_MM, AMP_MIN_PX, AMP_MAX_PX);
    float cycles =
        mapFloat(d2, IN_MIN_MM, IN_MAX_MM, FREQ_MIN_CYCLES, FREQ_MAX_CYCLES);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    display.print("A:");
    if (valid1)
      display.print((int)last1);
    else
      display.print("Na");
    display.print("  F:");
    if (valid2)
      display.print((int)last2);
    else
      display.print("Nb");
    display.println();

    display.setCursor(0, 10);
    display.print("Amp:");
    display.print((int)amplitudePx);
    display.print(" Cyc:");
    display.print(cycles, 2);

    int16_t centerY = SCREEN_HEIGHT / 2;
    display.drawLine(0, centerY, SCREEN_WIDTH - 1, centerY, SSD1306_WHITE);

    int16_t prevX = 0, prevY = centerY;
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      float phase = ((float)x / (SCREEN_WIDTH - 1)) * (2.0f * PI * cycles);
      int16_t y = centerY - (int16_t)(sinf(phase) * amplitudePx);
      if (x)
        display.drawLine(prevX, prevY, x, y, SSD1306_WHITE);
      prevX = x;
      prevY = y;
    }

    display.display();
  }
}
