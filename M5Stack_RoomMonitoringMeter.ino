#include <M5Stack.h>
#include <Wire.h>
#include <WiFi.h>
#include "time.h"
#include "Adafruit_Si7021.h" // https://github.com/adafruit/Adafruit_Si7021
#include "Seeed_BMP280.h" // https://github.com/Seeed-Studio/Grove_BMP280
#include "MHZ19.h" // https://github.com/WifWaf/MH-Z19

#define RX_PIN 16
#define TX_PIN 17
#define BAUDRATE 9600

const char* ssid       = "YOUR_SSID";
const char* password   = "YOUR_PASSWORD";
const char* ntpServer =  "ntp.jst.mfeed.ad.jp";
const long  gmtOffset_sec = 9 * 3600;
const int   daylightOffset_sec = 0;

// Humid sensor
Adafruit_Si7021 humidSensor = Adafruit_Si7021();

// Barometer sensor
BMP280 barometerSensor;

// Co2 sensor
MHZ19 co2Sensor;
HardwareSerial co2SensorSerial(1);
void verifyRange(int range);

void setup() {
  M5.begin();
  Serial.begin(115200);
  
  // wait for serial port to open
  while (!Serial) {
    delay(10);
  }

  // setup humid sensor
  if (!humidSensor.begin()) {
    Serial.println("Did not find Si7021 humid sensor");
    while (true);
  }

  // setup barometer sensor
  if (!barometerSensor.init()) {
    Serial.println("Did not find BMP280 barometer sensor");
    while (true);
  }

  // setup co2 sensor
  co2SensorSerial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);
  co2Sensor.begin(co2SensorSerial);
//  co2Sensor.setRange(2000);
//  co2Sensor.setSpan(2000);
  co2Sensor.autoCalibration(true);

  // setup screen
  M5.Lcd.clear();

  // draw the background image
  M5.Lcd.drawPngFile(SD, "/meter-bg.png", 0, 0);

  //　connect wifi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");

  // setup timer
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  //　disconnect wifi
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void loop() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  float tempH = humidSensor.readTemperature();
  float humid = humidSensor.readHumidity();
  float pressure = barometerSensor.getPressure();
  float tempB = barometerSensor.getTemperature();
  float altitude = barometerSensor.calcAltitude(pressure);
  int16_t co2Unlimited = co2Sensor.getCO2(true, true);
  int16_t co2Limited = co2Sensor.getCO2(false, true);
  float tempC = co2Sensor.getTemperature();
  int16_t co2 = co2Limited;

  int numXpos = 150;

  // display date
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  M5.Lcd.setCursor(20, 10);
  M5.Lcd.println(&timeinfo, "%Y/%m/%d %a %H:%M:%S");

  // display nums
  M5.Lcd.setTextSize(4);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

  M5.Lcd.setCursor(numXpos, 42);
  M5.Lcd.print((tempH + tempB) / 2, 1);

  M5.Lcd.setCursor(numXpos, 92);
  M5.Lcd.setTextColor(getHumidStatusColor(humid), TFT_BLACK);
  M5.Lcd.print(humid, 1);

  M5.Lcd.setCursor(numXpos, 142);
  M5.Lcd.setTextColor(getVentilationStatusColor(co2), TFT_BLACK);
  M5.Lcd.print(co2);

  M5.Lcd.setCursor(numXpos, 191);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.print(pressure / 100, 1);

  // temp log
  Serial.print("Humid Sensor: ");                      
  Serial.println(tempH);
  Serial.print("Barameter Sensor: ");                      
  Serial.println(tempB);
  Serial.print("CO2 Sensor: ");                      
  Serial.println(tempC);
  Serial.println(getVentilationStatusColor(1000));
  Serial.println("-------------");

  delay(1000);
}

void verifyRange(int range)
{
  Serial.println("Requesting new range.");
  co2Sensor.setRange(range);
  if (co2Sensor.getRange() == range)
    Serial.println("Range successfully applied.");
  else
    Serial.println("Failed to apply range.");
}

uint16_t getColor(uint8_t red, uint8_t green, uint8_t blue){
  return ((red>>3)<<11) | ((green>>2)<<5) | (blue>>3);
}

uint16_t getVentilationStatusColor(int16_t ppm) {
  if (ppm < 400)
    return getColor(0, 0, 255);
  else if (ppm < 500)
    return getColor(0, 255, 0);
  else if (ppm < 1000)
    return getColor(255, 255, 0);
  else if (ppm < 1500)
    return getColor(255, 145, 0);
  else if (ppm < 5000)
    return getColor(255, 0, 0);
  else
    return getColor(0, 0, 255);
}

uint16_t getHumidStatusColor(float humid) {
  if (humid <= 20)
    return getColor(255, 0, 0);
  else if (humid <= 30)
    return getColor(255, 145, 0);
  else if (humid <= 40)
    return getColor(255, 255, 0);
  else if (humid <= 50)
    return getColor(0, 255, 0);
  else if (humid <= 60)
    return getColor(255, 145, 0);
  else if (humid <= 70)
    return getColor(255, 255, 0);
  else if (humid <= 190)
    return getColor(255, 0, 0);
  else
    return getColor(0, 0, 255);
}
