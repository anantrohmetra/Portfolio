#include <Adafruit_VL53L0X.h>
#include <Wire.h>

#define SDA_PIN 18
#define SCL_PIN 23
#define XSHUT_1 14
#define XSHUT_2 27

Adafruit_VL53L0X lox1;
Adafruit_VL53L0X lox2;

void setup() {
  // Faster serial
  Serial.begin(115200);
  delay(1);

  // I2C on chosen pins, then faster bus
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(1000000); // 1 MHz on ESP32 (already fast)

  // Prepare XSHUT pins
  pinMode(XSHUT_1, OUTPUT);
  pinMode(XSHUT_2, OUTPUT);
  digitalWrite(XSHUT_1, LOW);
  digitalWrite(XSHUT_2, LOW);
  delay(1);

  // Bring up #1 and set unique address
  digitalWrite(XSHUT_1, HIGH);
  delay(1);
  if (!lox1.begin(0x30, &Wire)) {
    Serial.println("❌ VL53L0X #1 not detected!");
    while (1) {
      delay(2);
    }
  }
  // High-speed preset (lower timing budget)
  lox1.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_SPEED);

  // EXTRA SPEED: explicitly lower timing budget even more
  // Default high-speed is ~20–33 ms; 14000 µs ≈ 14 ms
  lox1.setMeasurementTimingBudgetMicroSeconds(14000);

  // Bring up #2 and set unique address
  digitalWrite(XSHUT_2, HIGH);
  delay(1);
  if (!lox2.begin(0x31, &Wire)) {
    Serial.println("❌ VL53L0X #2 not detected!");
    while (1) {
      delay(2);
    }
  }
  lox2.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_SPEED);
  lox2.setMeasurementTimingBudgetMicroSeconds(14000); // same here

  Serial.println("✅ Both VL53L0X Ready (high-speed + reduced timing budget)");
}

void loop() {
  VL53L0X_RangingMeasurementData_t m1, m2;

  // Single-shot measurements (fast timing budget)
  lox1.rangingTest(&m1, false);
  lox2.rangingTest(&m2, false);

  // Build one small buffer and write once (less Serial overhead)
  char out[24];
  int len = 0;

  if (m1.RangeStatus != 4) {
    // example: "523a"
    len += snprintf(out + len, sizeof(out) - len, "%ua",
                    (unsigned)m1.RangeMilliMeter);
  } else {
    len += snprintf(out + len, sizeof(out) - len, "Na");
  }

  if (m2.RangeStatus != 4) {
    len += snprintf(out + len, sizeof(out) - len, "%ub",
                    (unsigned)m2.RangeMilliMeter);
  } else {
    len += snprintf(out + len, sizeof(out) - len, "Nb");
  }

  if (len < (int)sizeof(out) - 2) {
    out[len++] = '\n';
    out[len] = '\0';
  }

  Serial.write((const uint8_t *)out, len);
}
