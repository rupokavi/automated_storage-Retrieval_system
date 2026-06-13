/*
 * ============================================================
 *   Conveyor Belt with Automated Storage and Retrieval System
 *   (ASRS) — Color-Based Sorting
 * ============================================================
 *
 *   Microcontroller : ESP32
 *   Color Sensor    : TCS34725 (via I2C)
 *   Display         : 16x2 LCD (I2C, address 0x27)
 *   Servo Library   : ESP32Servo
 *
 *   Authors:
 *     Punno Chandra Saha       (2005002)
 *     Md. Rafiul Islam         (2005004)
 *     Md. Ferdaous Al-Farabe   (2005023)
 *     Habibur Rahman Alamin    (2005025)
 *     Ifte kharul Islam Nahid  (2005034)
 *     Rupok Islam Avi          (2005058)
 *
 *   Department of Industrial & Production Engineering
 *   Rajshahi University of Engineering & Technology (RUET)
 *   Date: September 16, 2025
 *
 *   Supervisors:
 *     Sonia Akhter (Associate Professor)
 *     Md. Limonur Rahman Lingkon (Lecturer)
 *
 * ============================================================
 *   Libraries Required (install via Arduino Library Manager):
 *     - Adafruit TCS34725
 *     - LiquidCrystal I2C
 *     - ESP32Servo
 * ============================================================
 */

#include <Wire.h>
#include <Adafruit_TCS34725.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// ─────────────────────────────────────────────
//  Pin Definitions
// ─────────────────────────────────────────────
#define MOTOR_PIN    23   // Conveyor belt motor (relay, inverted logic)
#define SERVO_PIN    18   // Sorting servo
#define RELAY1_PIN   13   // Bin 1 DC motor relay
#define RELAY2_PIN   12   // Bin 2 DC motor relay
#define RELAY3_PIN   14   // Bin 3 DC motor relay
#define IR1_PIN      19   // IR Sensor 1 — object detection (active LOW)
#define IR2_PIN       5   // IR Sensor 2 — bin full detection (active LOW)
#define BUZZER_PIN    4   // Buzzer

// ─────────────────────────────────────────────
//  Servo Angles per Bin
// ─────────────────────────────────────────────
#define SERVO_BIN1   67   // Angle for Color 1
#define SERVO_BIN2   98   // Angle for Color 2
#define SERVO_BIN3  130   // Angle for Color 3

// ─────────────────────────────────────────────
//  Color Matching Threshold
//  (Euclidean distance in normalized RGB space)
//  Decrease for stricter matching, increase for
//  more tolerance. Tune after testing.
// ─────────────────────────────────────────────
#define MAX_COLOR_DIST 0.05

// ─────────────────────────────────────────────
//  Component Initialization
// ─────────────────────────────────────────────
Adafruit_TCS34725 tcs = Adafruit_TCS34725(
  TCS34725_INTEGRATIONTIME_50MS,
  TCS34725_GAIN_4X
);

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo;

// ─────────────────────────────────────────────
//  Color Storage
// ─────────────────────────────────────────────
struct Color {
  float r, g, b;
};

Color calibratedColors[3];   // Stores 3 calibrated reference colors
int   currentColorIndex = 0; // Index during calibration
int   sortedCounts[3] = {0, 0, 0}; // Per-bin sorted item counts

// ─────────────────────────────────────────────
//  System State Machine
// ─────────────────────────────────────────────
enum State { CALIBRATING, RUNNING, UNKNOWN_COLOR, BIN_FULL };
State systemState = CALIBRATING;

// ─────────────────────────────────────────────
//  Runtime Flags
// ─────────────────────────────────────────────
bool waitingForObjectPass = false;
int  lastDetectedColor    = -1;

// ─────────────────────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────────────────────
void setup() {
  // Pin modes
  pinMode(MOTOR_PIN,   OUTPUT);
  pinMode(RELAY1_PIN,  OUTPUT);
  pinMode(RELAY2_PIN,  OUTPUT);
  pinMode(RELAY3_PIN,  OUTPUT);
  pinMode(BUZZER_PIN,  OUTPUT);
  pinMode(IR1_PIN,     INPUT);
  pinMode(IR2_PIN,     INPUT);

  // Initial output states
  digitalWrite(MOTOR_PIN,   HIGH);  // Motor OFF (inverted logic)
  digitalWrite(RELAY1_PIN,  LOW);
  digitalWrite(RELAY2_PIN,  LOW);
  digitalWrite(RELAY3_PIN,  LOW);
  digitalWrite(BUZZER_PIN,  LOW);

  Serial.begin(115200);
  Serial.println("=== ASRS Color Sorter Booting ===");

  // TCS34725 color sensor
  if (!tcs.begin()) {
    Serial.println("[ERROR] TCS34725 not found! Check wiring.");
    while (1); // Halt
  }
  Serial.println("[OK] Color sensor initialized.");

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Place 1st color");

  // Servo
  servo.attach(SERVO_PIN);
  Serial.println("[OK] Servo attached.");
  Serial.println("[CALIBRATING] Waiting for 1st color object...");
}

// ─────────────────────────────────────────────────────────────
//  MAIN LOOP — State Machine Dispatcher
// ─────────────────────────────────────────────────────────────
void loop() {
  switch (systemState) {
    case CALIBRATING:    handleCalibration();   break;
    case RUNNING:        handleRunning();        break;
    case UNKNOWN_COLOR:  handleUnknownColor();   break;
    case BIN_FULL:       handleBinFull();        break;
  }
}

// ─────────────────────────────────────────────────────────────
//  COLOR SENSOR — Read & Average 20 Normalized RGB Readings
// ─────────────────────────────────────────────────────────────
Color readAverageColor() {
  float rSum = 0, gSum = 0, bSum = 0, cSum = 0;
  uint16_t r, g, b, c;

  for (int i = 0; i < 20; i++) {
    tcs.getRawData(&r, &g, &b, &c);
    rSum += r;
    gSum += g;
    bSum += b;
    cSum += c;
    delay(100); // Stability delay between readings
  }

  Color avg;
  avg.r = rSum / 20.0;
  avg.g = gSum / 20.0;
  avg.b = bSum / 20.0;
  float avgC = cSum / 20.0;

  // Normalize by clear channel to reduce ambient light effects
  if (avgC > 0) {
    avg.r /= avgC;
    avg.g /= avgC;
    avg.b /= avgC;
  }

  Serial.print("[SENSOR] Normalized RGB: ");
  Serial.print(avg.r, 4); Serial.print(", ");
  Serial.print(avg.g, 4); Serial.print(", ");
  Serial.println(avg.b, 4);

  return avg;
}

// ─────────────────────────────────────────────────────────────
//  COLOR MATCHING — Euclidean Distance
// ─────────────────────────────────────────────────────────────
float colorDistance(Color c1, Color c2) {
  return sqrt(
    pow(c1.r - c2.r, 2) +
    pow(c1.g - c2.g, 2) +
    pow(c1.b - c2.b, 2)
  );
}

// Returns index 0–2 of closest calibrated color,
// or -1 if distance exceeds MAX_COLOR_DIST threshold.
int findClosestColor(Color detected) {
  float minDist = colorDistance(detected, calibratedColors[0]);
  int   closest = 0;

  for (int i = 1; i < 3; i++) {
    float dist = colorDistance(detected, calibratedColors[i]);
    if (dist < minDist) {
      minDist = dist;
      closest = i;
    }
  }

  Serial.print("[MATCH] Min distance: ");
  Serial.println(minDist, 4);

  if (minDist > MAX_COLOR_DIST) {
    Serial.println("[MATCH] Result: UNKNOWN (distance too large)");
    return -1;
  }

  Serial.print("[MATCH] Result: Color ");
  Serial.println(closest + 1);
  return closest;
}

// ─────────────────────────────────────────────────────────────
//  LCD — Running Status with Sorted Counts
// ─────────────────────────────────────────────────────────────
void updateLCDCounts() {
  lcd.clear();
  lcd.print("Running...");
  lcd.setCursor(0, 1);
  lcd.print("C1:");
  lcd.print(sortedCounts[0]);
  lcd.print(" C2:");
  lcd.print(sortedCounts[1]);
  lcd.print(" C3:");
  lcd.print(sortedCounts[2]);
}

// ─────────────────────────────────────────────────────────────
//  STATE: CALIBRATING
//  Scans 3 reference colors one by one.
//  Object must be placed, then removed to proceed.
// ─────────────────────────────────────────────────────────────
void handleCalibration() {
  static bool waitingForObject = true;

  // Object placed → scan
  if (waitingForObject && digitalRead(IR1_PIN) == LOW) {
    Serial.println("[CALIB] Object detected, scanning...");
    delay(50); // IR debounce

    Color avgColor = readAverageColor();
    calibratedColors[currentColorIndex] = avgColor;

    lcd.clear();
    lcd.print("Scan complete");
    lcd.setCursor(0, 1);
    lcd.print("Remove object");
    waitingForObject = false;
  }

  // Object removed → advance to next color or start running
  else if (!waitingForObject && digitalRead(IR1_PIN) == HIGH) {
    Serial.println("[CALIB] Object removed.");
    delay(50); // IR debounce

    currentColorIndex++;

    if (currentColorIndex < 3) {
      lcd.clear();
      lcd.print("Place ");
      lcd.print(currentColorIndex + 1);
      lcd.print(currentColorIndex == 1 ? "nd color" : "rd color");
      waitingForObject = true;
      Serial.print("[CALIB] Waiting for color ");
      Serial.println(currentColorIndex + 1);
    }
    else {
      // All 3 colors calibrated — start system
      lcd.clear();
      lcd.print("Calibration done");
      lcd.setCursor(0, 1);
      lcd.print("Starting...");
      delay(2000);

      digitalWrite(MOTOR_PIN, LOW); // Start conveyor (inverted logic)
      systemState = RUNNING;
      updateLCDCounts();
      Serial.println("[CALIB] Complete. Conveyor started. System RUNNING.");
    }
  }
}

// ─────────────────────────────────────────────────────────────
//  STATE: RUNNING
//  Detects objects, classifies color, positions servo,
//  checks bin capacity, and counts sorted items.
// ─────────────────────────────────────────────────────────────
void handleRunning() {

  // ── Rising edge: object has fully passed the sensor ──
  if (waitingForObjectPass && digitalRead(IR1_PIN) == HIGH) {
    delay(50); // Debounce
    waitingForObjectPass = false;

    if (lastDetectedColor != -1) {
      sortedCounts[lastDetectedColor]++;
      updateLCDCounts();
      Serial.println("[SORT] Object passed. Count incremented.");
    }

    lastDetectedColor = -1;
  }

  // ── Falling edge: new object detected on belt ──
  if (!waitingForObjectPass && digitalRead(IR1_PIN) == LOW) {
    Serial.println("[DETECT] Object on belt.");
    delay(50); // Debounce

    Color detectedColor  = readAverageColor();
    int   closestColor   = findClosestColor(detectedColor);

    // Unknown color — halt and alert
    if (closestColor == -1) {
      digitalWrite(MOTOR_PIN, HIGH); // Stop belt
      digitalWrite(BUZZER_PIN, HIGH);
      lcd.clear();
      lcd.print("Unknown color!");
      lcd.setCursor(0, 1);
      lcd.print("Remove object");
      systemState = UNKNOWN_COLOR;
      Serial.println("[ALERT] Unknown color. Belt stopped.");
      return;
    }

    // Show detected color on LCD briefly
    lcd.clear();
    lcd.print("Detected C");
    lcd.print(closestColor + 1);
    delay(1000);
    updateLCDCounts();

    // Move servo to corresponding bin angle
    int angle, relayPin;
    if (closestColor == 0) {
      angle    = SERVO_BIN1;
      relayPin = RELAY1_PIN;
    } else if (closestColor == 1) {
      angle    = SERVO_BIN2;
      relayPin = RELAY2_PIN;
    } else {
      angle    = SERVO_BIN3;
      relayPin = RELAY3_PIN;
    }

    servo.write(angle);
    Serial.print("[SERVO] Moving to angle: ");
    Serial.println(angle);
    delay(500); // Allow servo to settle

    // Check if bin is full after object arrives
    delay(100);
    if (digitalRead(IR2_PIN) == LOW) {
      // Bin full — halt system
      digitalWrite(MOTOR_PIN, HIGH); // Stop belt
      digitalWrite(BUZZER_PIN, HIGH);
      digitalWrite(relayPin,   HIGH);

      lcd.clear();
      lcd.print("Bin ");
      lcd.print(closestColor + 1);
      lcd.print(" full!");
      lcd.setCursor(0, 1);
      lcd.print("Empty it");

      systemState = BIN_FULL;
      Serial.println("[ALERT] Bin full. Belt stopped.");
    }
    else {
      // Bin not full — wait for this object to clear the sensor
      waitingForObjectPass = true;
      lastDetectedColor    = closestColor;
      Serial.println("[RUNNING] Waiting for object to pass...");
    }
  }
}

// ─────────────────────────────────────────────────────────────
//  STATE: UNKNOWN_COLOR
//  Waits for operator to remove the unrecognized object.
// ─────────────────────────────────────────────────────────────
void handleUnknownColor() {
  if (digitalRead(IR1_PIN) == HIGH) { // Object removed
    delay(50); // Debounce
    digitalWrite(MOTOR_PIN,  LOW);  // Restart belt
    digitalWrite(BUZZER_PIN, LOW);  // Silence buzzer
    updateLCDCounts();
    systemState = RUNNING;
    Serial.println("[RESUME] Unknown object removed. Resuming.");
  }
}

// ─────────────────────────────────────────────────────────────
//  STATE: BIN_FULL
//  Waits for operator to empty the full bin.
//  Resumes after 5-second confirmation delay.
// ─────────────────────────────────────────────────────────────
void handleBinFull() {
  static unsigned long emptyTime     = 0;
  static bool          waitingForEmpty = true;

  // IR2 goes HIGH → bin has been emptied
  if (waitingForEmpty && digitalRead(IR2_PIN) == HIGH) {
    delay(50); // Debounce
    emptyTime      = millis();
    waitingForEmpty = false;
    Serial.println("[BIN] Bin emptied. Waiting 5s before resume...");
  }

  // 5-second grace period before restarting
  else if (!waitingForEmpty && (millis() - emptyTime >= 5000)) {
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(RELAY1_PIN, LOW);
    digitalWrite(RELAY2_PIN, LOW);
    digitalWrite(RELAY3_PIN, LOW);
    digitalWrite(MOTOR_PIN,  LOW); // Restart belt

    updateLCDCounts();
    systemState     = RUNNING;
    waitingForEmpty = true;
    Serial.println("[RESUME] Belt restarted after bin empty.");
  }
}
