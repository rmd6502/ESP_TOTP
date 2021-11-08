#include "AiEsp32RotaryEncoder.h"
#include <BleKeyboard.h>
#define _WIFIMGR_LOGLEVEL_    3
#include <ESP_WiFiManager.h>
#include <NTPClient.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <TOTP.h>
#include <TFT_eSPI.h>
#include <WiFi.h>

#include <driver/rtc_io.h>

#include "secrets.h"

#define ROTARY_ENCODER_A_PIN 26
#define ROTARY_ENCODER_B_PIN 27
#define ROTARY_ENCODER_BUTTON_PIN 25
#define ROTARY_ENCODER_VCC_PIN 33 /* 27 put -1 of Rotary encoder Vcc is connected directly to 3,3V; else you can use declared output pin for powering rotary encoder */
#define ROTARY_ENCODER_GND_PIN 32
#define ROTARY_ENCODER_STEPS 4
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

BleKeyboard kb;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
uint32_t lastInputTime = 0;

#define SLEEP_TIME 10000

char buff[512];

std::vector<Secret> secrets;
uint8_t selected = 0;

void menuize(int curr, std::vector<Secret> &secrets);

void rotary_loop()
{
  //dont print anything unless value changed
  if (rotaryEncoder.encoderChanged())
  {
    lastInputTime = millis();
    Serial.print("Value: ");
    Serial.println(rotaryEncoder.readEncoder());
    menuize(rotaryEncoder.readEncoder(), secrets);
  }
  if (rotaryEncoder.isEncoderButtonClicked())
  {
    lastInputTime = millis();
    rotary_onButtonClick();
  }
}

void IRAM_ATTR readEncoderISR()
{
  rotaryEncoder.readEncoder_ISR();
}

void menuize(int curr, std::vector<Secret> &secrets) {
  static int last = -1;
  static int menuoffset = 0;
  static int screenHeight = floor(tft.height()/tft.fontHeight()) - 1;
  bool offsetChanged = false;
  curr = max(0, min((int)secrets.size() - 1, curr));
  Serial.print("font height "); Serial.println(tft.fontHeight());
  Serial.print("Curr "); Serial.println(curr);
  Serial.print("menuoffset "); Serial.println(menuoffset);
  Serial.print("screenHeight "); Serial.println(screenHeight);
  
  if (curr < menuoffset) {
    menuoffset = curr;
    offsetChanged = true;
  } else if (curr + menuoffset > screenHeight) {
    menuoffset = curr - screenHeight;
    offsetChanged = true;
  }

  if (offsetChanged || last == -1) {
    int currentLine = menuoffset;
    for (int i=0; i < min(screenHeight, (int)secrets.size() - menuoffset); ++i) {
      if (currentLine == curr) {
        tft.setTextColor(TFT_BLACK, TFT_PINK);
      } else {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
      }
      tft.drawString(secrets[currentLine++].issuer.c_str(), 0, i * tft.fontHeight() + 8);
    }
  } else {
    //tft.fillRoundRect(0,(curr - menuoffset) * tft.fontHeight(),tft.textWidth(secrets[curr].issuer.c_str()),8,2,TFT_PINK);
    tft.setTextColor(TFT_BLACK, TFT_PINK);
    tft.drawString(secrets[curr].issuer.c_str(),0,(curr - menuoffset) * tft.fontHeight() + 8);
    if (last > -1 && last != curr && last >= menuoffset && last + menuoffset <= screenHeight) {
      //tft.fillRoundRect(0,(last - menuoffset) * tft.fontHeight(),tft.textWidth(secrets[last].issuer.c_str()),8,2,TFT_BLACK);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.drawString(secrets[last].issuer.c_str(),0,last * tft.fontHeight() + 8);
    }
  }
  selected = curr;
  last = curr;
}

void rotary_onButtonClick() {
  auto secret = secrets[selected];
  auto data = secret.secretBytes.data();
  TOTP totp(data, secret.secretBytes.size() , 30);

  auto code = totp.getCode(timeClient.getEpochTime());
  Serial.print("Code ");
  Serial.println(code);
  if (kb.isConnected()) {
    kb.print(code);
  }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Start");
    Serial.println(esp_sleep_get_wakeup_cause());
    rtc_gpio_deinit(GPIO_NUM_25);
    rtc_gpio_deinit(GPIO_NUM_32);
    rtc_gpio_deinit(GPIO_NUM_33);
    Serial.print("gpio25 "); Serial.println(digitalRead(25));
    
    kb.begin();

    pinMode(ROTARY_ENCODER_GND_PIN, OUTPUT);
    digitalWrite(ROTARY_ENCODER_GND_PIN, LOW);
    rotaryEncoder.begin();
    rotaryEncoder.setup(readEncoderISR);

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(0, 0);
    tft.setTextDatum(MC_DATUM);

    /*
    if (TFT_BL > 0) {                           // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
        pinMode(TFT_BL, OUTPUT);                // Set backlight pin to output mode
        digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
    }
    */

    tft.setSwapBytes(true);

    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);

    SPIFFS.begin();
    secrets = readSecrets();
    bool circleValues = true;
    rotaryEncoder.setBoundaries(0, secrets.size(), circleValues);
    menuize(0, secrets);
    ESP_WiFiManager ESP_wifiManager("TOTPAP");
    ESP_wifiManager.autoConnect("TOTPAP");
    lastInputTime = millis();
}

void loop()
{
  if (millis() - lastInputTime >= SLEEP_TIME) {
    Serial.println("Sleep");
    Serial.print("gpio25 "); Serial.println(digitalRead(25));
    digitalWrite(TFT_BL, LOW);
    tft.writecommand(TFT_DISPOFF);
    tft.writecommand(TFT_SLPIN);
    //esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    //rtc_gpio_pullup_en(GPIO_NUM_25);
    rtc_gpio_pullup_en(GPIO_NUM_33);
    rtc_gpio_pulldown_en(GPIO_NUM_32);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_25, 0);
    esp_deep_sleep_start();
  }
    rotary_loop();
    // update the time 
    timeClient.update();
}
