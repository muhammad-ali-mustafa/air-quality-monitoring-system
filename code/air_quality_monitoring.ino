

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---- LCD (16x2 I2C) ----
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---- DHT22 ----
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ---- MQ135 ----
int mq135Pin = A0;

// ---- AQI thresholds ----
int aqiModerate = 300;
int aqiBad = 400;

// ---- LED (Hardware + Blynk) ----
int ledPin = 14;  // D5 on NodeMCU, GPIO14

// ---- WiFi ----
char auth[] = BLYNK_AUTH_TOKEN;

// ---- Timers ----
unsigned long previousMillis = 0;
const long interval = 5000;  

unsigned long lcdPreviousMillis = 0;
const long lcdInterval = 3000;
bool showTempHum = true;

// ---- Function to update LCD (MODIFIED HERE) ----
void updateLCD(float t, float h, int gasValue, float oxygenValue) {
  unsigned long currentMillis = millis();
  if (currentMillis - lcdPreviousMillis >= lcdInterval) {
    lcdPreviousMillis = currentMillis;
    showTempHum = !showTempHum;
  }

  lcd.clear();
  if (showTempHum) {
    // Screen 1: Temperature + Humidity
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(t);
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("Hum:  ");
    lcd.print(h);
    lcd.print("%");
  } 
  else {
    // Screen 2: AQI + Oxygen (Status Removed)
    lcd.setCursor(0, 0);
    lcd.print("AQI: ");
    lcd.print(gasValue);

    lcd.setCursor(0, 1);
    lcd.print("O2: ");
    lcd.print(oxygenValue, 1);
    lcd.print("%");
  }
}

// ---- Function to read sensors and send to Blynk ----
void sendSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int gasValue = analogRead(mq135Pin);

  // Oxygen decreases when AQI increases
  float oxygenValue = 30 - (gasValue * 0.04);
  if (oxygenValue < 0) oxygenValue = 0;

  // Send to Blynk
  if (!isnan(t) && !isnan(h)) {
    Blynk.virtualWrite(V0, t);
    Blynk.virtualWrite(V1, h);
  }
  Blynk.virtualWrite(V2, gasValue);
  Blynk.virtualWrite(V4, oxygenValue);  // NEW: Oxygen on Blynk

  // LED + Alert logic (unchanged)
  if (gasValue < aqiModerate) {
    digitalWrite(ledPin, LOW);
    Blynk.virtualWrite(V3, 0);
  }
  else if (gasValue >= aqiModerate && gasValue < aqiBad) {
    digitalWrite(ledPin, HIGH);
    Blynk.virtualWrite(V3, 255);
  }
  else {
    digitalWrite(ledPin, HIGH);
    Blynk.virtualWrite(V3, 255);
    Blynk.logEvent("pollution_alert", "Bad Air!");
  }

  // Update LCD (status removed, oxygen added)
  updateLCD(t, h, gasValue, oxygenValue);

  // Serial Debug
  Serial.print("T: "); Serial.print(t);
  Serial.print("  H: "); Serial.print(h);
  Serial.print("  Gas: "); Serial.print(gasValue);
  Serial.print("  O2: "); Serial.println(oxygenValue);
}

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Air Quality");
  lcd.setCursor(0, 1);
  lcd.print("Monitoring...");
  delay(2000);
  lcd.clear();

  dht.begin();
  Blynk.begin(auth, ssid, pass);
}

void loop() {
  Blynk.run();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sendSensor();
  }
}
