/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
   Adapted from github moononournation
   Added CO2 Measurements with an SGP30 Sensor and LED Matrix for #CO2Ampel @Ulrich Spizig in 2020

Sketch performs a Scan for  Bluetooth Low Energy Devices(BLE). If a found UUID of a BLE Device is equal to German Corona Warn App is sends this to Serial Monitor


*/

#define SGP30_CONNECTED
#define M5ATOM
#define SCAN_TIME 30     // seconds
#define WAIT_WIFI_LOOP 5 // around 4 seconds for 1 loop
#define SLEEP_TIME 90   // seconds
#define SCAN_REPETITION_TIME 90000
 
#include <Arduino.h>
//#include <sstream>

#include "credentials.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <WiFi.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <Wire.h>
#include "Adafruit_SGP30.h"

#ifdef SGP30_CONNECTED
  #define sda 26 ///* I2C Pin Definition */
  #define scl 32 ///* I2C Pin Definition */
  
  Adafruit_SGP30 sgp;
  
  float sgp30_CO2 = 400;
  float sgp30_TVOC = 2;
  int counter = 0;
#endif

#ifdef M5ATOM
  #include <FastLED.h>
   // Define the array of leds
  #define NUM_LEDS 25 
  #define DATA_PIN 27
  FASTLED_USING_NAMESPACE
  #define MAX_POWER_MILLIAMPS 500
  CRGB leds[NUM_LEDS]; 
#endif


bool data_sent = false;
int wait_wifi_counter = 0;
int corona_app_counter = 0;
long lastMsg = 0;
int corona_app_counts = 0;
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    /*Serial.print("Advertised Device:"); 
    Serial.println(advertisedDevice.toString().c_str());*/
  }
};

void setup_ai_sensor(void){
        Wire.begin(sda, scl);
        if (! sgp.begin()){
          Serial.println("SGP30 eCo2 Sensor not found :(");
          return;
        }
        Serial.print("Found SGP30 eCo2 Sensor - Serial #");
        Serial.print(sgp.serialnumber[0], HEX);
        Serial.print(sgp.serialnumber[1], HEX);
        Serial.println(sgp.serialnumber[2], HEX);   
        // If you have a baseline measurement from before you can assign it to start, to 'self-calibrate'
        //sgp.setIAQBaseline(0x8E68, 0x8F41);  // Will vary for each sensor!
}
#ifdef SGP30_CONNECTED
  void sgp30Messung(void){
      if (! sgp.IAQmeasure()) {
        Serial.println("Measurement failed");
        return;
      }
      Serial.print("TVOC "); Serial.print(sgp.TVOC); Serial.print(" ppb\t");
      Serial.print("eCO2 "); Serial.print(sgp.eCO2); Serial.print(" ppm\t");
    
      if (! sgp.IAQmeasureRaw()) {
        Serial.println("Raw Measurement failed");
        return;
      }
      Serial.print("Raw H2 "); Serial.print(sgp.rawH2); Serial.print(" \t"); Serial.print("Raw Ethanol "); Serial.print(sgp.rawEthanol); Serial.println("");
     sgp30_CO2 = float(sgp.eCO2);
     sgp30_TVOC = float(sgp.TVOC);
      delay(1000);
    
      counter++;
      if (counter == 30) {
        counter = 0;
    
        uint16_t TVOC_base, eCO2_base;
        if (! sgp.getIAQBaseline(&eCO2_base, &TVOC_base)) {
          Serial.println("Failed to get baseline readings");
          return;
        }
        //Serial.print("****Baseline values: eCO2: 0x"); Serial.print(eCO2_base, HEX);
        //Serial.print(" & TVOC: 0x"); Serial.println(TVOC_base, HEX);
      }
  }
#endif

#ifdef M5ATOM
  /* M5 ATOM LED MATRIX ansteuern */
  void LEDUpdate(void){
    for(int i = 0; i < NUM_LEDS; i++) {
      if (sgp30_CO2 < 700)      leds[i] = CRGB::Green;
      else if (sgp30_CO2 < 1300) leds[i] = CRGB::Yellow;
      else                      leds[i] = CRGB::Red;
      FastLED.show(); 
    }
  }
  void LEDInit(void){
    LEDS.addLeds<WS2812,DATA_PIN,GRB>(leds,NUM_LEDS);
    LEDS.setBrightness(54);
    for(int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Blue;
    }
    FastLED.show(); 
  }
#endif

void setup()
{
  Serial.begin(115200);
  Serial.println("\nESP32 BLE Scanner");
  #ifdef SGP30_CONNECTED
    setup_ai_sensor();
  #endif
  //disable brownout detector to maximize battery life
  //Serial.print("disable brownout detector");
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.print("Init BLE");
  BLEDevice::init("");
  #ifdef M5ATOM
        LEDInit();
        LEDS.addLeds<WS2812,DATA_PIN,GRB>(leds,NUM_LEDS);
        LEDS.setBrightness(54);
  #endif

}

void bleScanforCorona(){
  
  BLEScan *pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(0x50);
  pBLEScan->setWindow(0x30);
  Serial.print("Scan started: ");Serial.println(millis());
  BLEScanResults foundDevices = pBLEScan->start(SCAN_TIME);
  Serial.print("Scan finished: ");Serial.println(millis());
  int count2 = foundDevices.getCount();
  for (int i = 0; i < count2; i++)
  {
    if (i > 0)
    {
      //Serial.print("i");
    }
    BLEAdvertisedDevice d = foundDevices.getDevice(i);
    if (d.haveServiceUUID())
    {
      String dev_uuid;
      String Address;
      int RSSIRX;
      dev_uuid = d.getServiceUUID().toString().c_str();
      if (dev_uuid.indexOf("000fd6f") >0){
        corona_app_counts++;
        Address = d.getAddress().toString().c_str();
        RSSIRX = d.getRSSI();
        Serial.print("Device Adress: ");Serial.print(Address);
        Serial.print(" Corona App-Service-UUID: ");Serial.println(dev_uuid);
      }
    }
  }
  Serial.println("Scan done!");
  Serial.print("Anzahl BT Geräte:"); Serial.println(count2);
  Serial.print("Anzahl Corona-App Geräte:"); Serial.println(corona_app_counts);  
  corona_app_counts = 0;
}

void loop()
{
  
        long now = millis();
        if (now - lastMsg > SCAN_REPETITION_TIME) {
          Serial.print("Time: ");Serial.println(lastMsg);
          lastMsg = now;
          #ifdef SGP30_CONNECTED
            sgp30Messung(); 
          #endif  
          #ifdef M5ATOM
            LEDUpdate();          
          #endif
          Serial.println("\nBLE Scan");
          bleScanforCorona();  
        }
}
