#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <BH1750.h>
#include <TinyGPS++.h>
#include <Adafruit_SSD1306.h>
#include "Adafruit_HTU21DF.h"

// Tambahan untuk WiFiManager + Firebase
#include <WiFiManager.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>

// ================== PIN SETTING ======================
#define SD_CS         5
#define GPS_RX        16
#define GPS_TX        17
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

// ================== OBJEK ============================
BH1750 lightMeter;
Adafruit_HTU21DF htu = Adafruit_HTU21DF();
TinyGPSPlus gps;
HardwareSerial GPSserial(1);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
File dataFile;

// ================== FIREBASE OBJECTS ==================
FirebaseConfig config;
FirebaseAuth auth;
FirebaseData fbdo;
FirebaseJson json;

// ================== VARIABEL GLOBAL ==================
float lat = 0.0, lng = 0.0, speed = 0.0, heading = 0.0;
int sats = 0;
float temperature = 0.0, humidity = 0.0;
String currentLogFile = "";
const int valX = 35;

/* ----------------- CONFIG FIREBASE (METODE DATABASE SECRET) ----------------- */
#define FIREBASE_DATABASE_URL "SECRET"
#define FIREBASE_DATABASE_SECRET "SECRET"
#define FIREBASE_DATA_PATH "/data_realtime"

// ================== TIMER NON-BLOCKING ==================
unsigned long lastSensorTime = 0;
unsigned long lastFirebaseTime = 0;
unsigned long lastSDTime = 0;

const unsigned long sensorInterval = 1000;     // Baca sensor setiap 1 detik
const unsigned long firebaseInterval = 1000;   // Kirim Firebase setiap 1 detik
const unsigned long sdInterval = 10000;        // Simpan SD Card setiap 10 detik

// ================== DEKLARASI FUNGSI ==================
void showStatusMessage(String message);
void drawStaticLabels();
String getDateTimeString(TinyGPSDate date, TinyGPSTime time);
String getDateFileName(TinyGPSDate date);
void checkOrCreateLogFile(String fileName);
bool sendToFirebase(String datetime, float lat, float lng, float speed, int sats, float heading, float lux, float temp, float hum);
void writeToSDCard(String dateTimeStr, float lat, float lng, float speed, int sats, float heading, float lux, float temp, float hum);
void updateOLEDValues(float lux);
void showDateTimeOLED(String dateTimeStr);
void displayGPSSearching();
String padZero(int num);

// ================== SETUP ======================
void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED gagal!");
    while (1);
  }
  display.clearDisplay();
  showStatusMessage("Memulai sistem...");
  delay(1000);

  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    showStatusMessage("BH1750 gagal!");
    while (1);
  } else showStatusMessage("BH1750 siap");

  if (!htu.begin()) {
    showStatusMessage("HTU21D gagal!");
    while (1);
  } else showStatusMessage("HTU21D siap");

  GPSserial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  showStatusMessage("GPS siap");

  if (!SD.begin(SD_CS)) {
    showStatusMessage("SD gagal!");
    while (1);
  } else showStatusMessage("SD Card siap");

  // === KONEKSI WiFiManager ===
  showStatusMessage("WiFiManager...");
  WiFiManager wm;
  if (!wm.autoConnect("ESP32_Config")) {
    Serial.println("WiFiManager gagal, restart...");
    delay(3000);
    ESP.restart();
  }
  showStatusMessage("WiFi terkoneksi");

  // === Inisialisasi Firebase ===
  showStatusMessage("Inisialisasi Firebase...");
  config.database_url = FIREBASE_DATABASE_URL;
  config.signer.tokens.legacy_token = FIREBASE_DATABASE_SECRET;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  showStatusMessage("Sistem siap");
  delay(800);

  display.clearDisplay();
  drawStaticLabels();
}

// ================== LOOP NON-BLOCKING ======================
void loop() {
  while (GPSserial.available()) gps.encode(GPSserial.read());
  unsigned long currentMillis = millis();

  // 1️⃣ BACA SENSOR SETIAP 1 DETIK
  static float lux = 0;
  if (currentMillis - lastSensorTime >= sensorInterval) {
    lastSensorTime = currentMillis;
    lux = lightMeter.readLightLevel();
    temperature = htu.readTemperature();
    humidity = htu.readHumidity();

    if (gps.location.isValid() && gps.date.isValid() && gps.time.isValid()) {
      lat = gps.location.lat();
      lng = gps.location.lng();
      speed = gps.speed.kmph();
      sats = gps.satellites.value();
      heading = gps.course.deg();

      String dateTimeStr = getDateTimeString(gps.date, gps.time);
      updateOLEDValues(lux);
      showDateTimeOLED(dateTimeStr);
    } else {
      displayGPSSearching();
    }
  }

  // 2️⃣ KIRIM DATA KE FIREBASE SETIAP 1 DETIK
  if (currentMillis - lastFirebaseTime >= firebaseInterval) {
    lastFirebaseTime = currentMillis;
    if (gps.location.isValid() && gps.date.isValid() && gps.time.isValid()) {
      String dateTimeStr = getDateTimeString(gps.date, gps.time);
      bool sent = sendToFirebase(dateTimeStr, lat, lng, speed, sats, heading, lux, temperature, humidity);
      if (!sent) Serial.println("Gagal kirim Firebase!");
    }
  }

  // 3️⃣ SIMPAN DATA KE SD CARD SETIAP 10 DETIK
  if (currentMillis - lastSDTime >= sdInterval) {
    lastSDTime = currentMillis;
    if (gps.location.isValid() && gps.date.isValid() && gps.time.isValid()) {
      String dateTimeStr = getDateTimeString(gps.date, gps.time);
      writeToSDCard(dateTimeStr, lat, lng, speed, sats, heading, lux, temperature, humidity);
    }
  }
}

// ================== FIREBASE FUNCTION ====================
bool sendToFirebase(String datetime, float lat, float lng, float speed, int sats, float heading, float lux, float temp, float hum) {
  if (WiFi.status() != WL_CONNECTED) return false;

  json.clear();
  json.add("DateTime", datetime);
  json.add("Latitude", lat);
  json.add("Longitude", lng);
  json.add("Speed_kmph", speed);
  json.add("Satellites", sats);
  json.add("Heading_deg", heading);
  json.add("Lux", lux);
  json.add("Temperature_C", temp);
  json.add("Humidity_pct", hum);

  if (Firebase.RTDB.setJSON(&fbdo, FIREBASE_DATA_PATH, &json)) {
    Serial.println("Firebase update OK");
    return true;
  } else {
    Serial.print("Firebase gagal: ");
    Serial.println(fbdo.errorReason());
    return false;
  }
}

// ================== SD CARD FUNCTION =====================
void writeToSDCard(String dateTimeStr, float lat, float lng, float speed, int sats, float heading, float lux, float temp, float hum) {
  String newLogFile = "/log_" + getDateFileName(gps.date) + ".txt";
  if (newLogFile != currentLogFile) {
    currentLogFile = newLogFile;
    checkOrCreateLogFile(currentLogFile);
  }

  if (SD.exists(currentLogFile)) {
    dataFile = SD.open(currentLogFile, FILE_APPEND);
    if (dataFile) {
      dataFile.print(dateTimeStr); dataFile.print(",");
      dataFile.print(lat, 6); dataFile.print(",");
      dataFile.print(lng, 6); dataFile.print(",");
      dataFile.print(speed, 2); dataFile.print(",");
      dataFile.print(sats); dataFile.print(",");
      dataFile.print(heading, 2); dataFile.print(",");
      dataFile.print(lux, 2); dataFile.print(",");
      dataFile.print(temp, 2); dataFile.print(",");
      dataFile.println(hum, 2);
      dataFile.close();
      Serial.println("--> Data disimpan ke " + currentLogFile);
    }
  }
}

// ================== FUNGSI TAMBAHAN ======================
String getDateTimeString(TinyGPSDate date, TinyGPSTime time) {
  char buffer[25];
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
          date.year(), date.month(), date.day(),
          time.hour(), time.minute(), time.second());
  return String(buffer);
}

String getDateFileName(TinyGPSDate date) {
  return String(date.year()) + padZero(date.month()) + padZero(date.day());
}

String padZero(int num) {
  return (num < 10 ? "0" : "") + String(num);
}

void checkOrCreateLogFile(String fileName) {
  if (!SD.exists(fileName)) {
    File f = SD.open(fileName, FILE_WRITE);
    if (f) {
      f.println("DateTime,Latitude,Longitude,Speed(km/h),Satellites,Heading,Lux,Temperature(C),Humidity(%)");
      f.close();
    }
  }
}

void drawStaticLabels() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);  display.print("T/H:");
  display.setCursor(0, 10); display.print("Lux:");
  display.setCursor(0, 20); display.print("Lat:");
  display.setCursor(0, 30); display.print("Lng:");
  display.setCursor(0, 40); display.print("Spd:");
  display.setCursor(70, 40); display.print("Sat:");
  display.setCursor(70, 50); display.print("Dir:");
  display.display();
}

void updateOLEDValues(float lux) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.fillRect(valX, 0, 90, 8, SSD1306_BLACK);
  display.setCursor(valX, 0);
  display.print(temperature, 1); display.print((char)247); display.print("C ");
  display.print(humidity, 1); display.print("%");

  display.fillRect(valX, 10, 90, 8, SSD1306_BLACK);
  display.setCursor(valX, 10); display.print(lux, 1);

  display.fillRect(valX, 20, 90, 8, SSD1306_BLACK);
  display.setCursor(valX, 20); display.print(lat, 4);

  display.fillRect(valX, 30, 90, 8, SSD1306_BLACK);
  display.setCursor(valX, 30); display.print(lng, 4);

  display.fillRect(valX, 40, 35, 8, SSD1306_BLACK);
  display.setCursor(valX, 40); display.print(speed, 1);

  display.fillRect(95, 40, 25, 8, SSD1306_BLACK);
  display.setCursor(95, 40); display.print(sats);

  display.fillRect(95, 50, 30, 8, SSD1306_BLACK);
  display.setCursor(95, 50); display.print(heading, 0); display.print((char)247);

  display.display();
}

void showDateTimeOLED(String dateTimeStr) {
  display.fillRect(0, 56, SCREEN_WIDTH, 8, SSD1306_BLACK);
  display.setCursor(0, 56);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.print(dateTimeStr);
  display.display();
}

void displayGPSSearching() {
  display.fillRect(0, 56, SCREEN_WIDTH, 8, SSD1306_BLACK);
  display.setCursor(0, 56);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.print("Mencari sinyal GPS...");
  display.display();
}

void showStatusMessage(String message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(message, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  int y = (SCREEN_HEIGHT - h) / 2;
  display.setCursor(x, y);
  display.print(message);
  display.display();
}
