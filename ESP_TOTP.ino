#include "AiEsp32RotaryEncoder.h"
#include <BleKeyboard.h>
#define _WIFIMGR_LOGLEVEL_    1
#include <ESP_WiFiManager.h>
#include <NTPClient.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <TOTP.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <vector>
#include <string>

#include <driver/rtc_io.h>

#include "menuize.h"
#include "secrets.h"

#define ROTARY_ENCODER_A_PIN 26
#define ROTARY_ENCODER_B_PIN 27
#define ROTARY_ENCODER_BUTTON_PIN 25
#define ROTARY_ENCODER_VCC_PIN 33 /* 27 put -1 of Rotary encoder Vcc is connected directly to 3,3V; else you can use declared output pin for powering rotary encoder */
#define ROTARY_ENCODER_GND_PIN 32
#define ROTARY_ENCODER_STEPS 4
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

Menu *mainMenu;
BleKeyboard kb("RMD_TOTP");
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
uint32_t lastInputTime = 0;
uint32_t lastReadTime = 0;
double lastBatteryReading = 0;

#define SLEEP_TIME 10000
#define BATTERY_READ_TIME 1000

char buff[512];

std::vector<Secret> secrets;

void rotary_loop()
{
  //dont print anything unless value changed
  if (rotaryEncoder.encoderChanged())
  {
    lastInputTime = millis();
    Serial.print("Value: ");
    Serial.println(rotaryEncoder.readEncoder());
    mainMenu->selectItem(rotaryEncoder.readEncoder());
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

void rotary_onButtonClick() {
  auto secret = secrets[mainMenu->selectedItem()];
  auto data = secret.secretBytes.data();
  TOTP totp(data, secret.secretBytes.size() , 30);
  timeClient.update();
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

    tft.init();

    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(0, 0);
    Serial.println("tft init");

    kb.begin();
    Serial.println("kb init");

    pinMode(ROTARY_ENCODER_GND_PIN, OUTPUT);
    digitalWrite(ROTARY_ENCODER_GND_PIN, LOW);
    rotaryEncoder.begin();
    rotaryEncoder.setup(readEncoderISR);
    Serial.println("rotary init");

    tft.setSwapBytes(true);

    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    tft.println("Reading Secrets");

    SPIFFS.begin();
    secrets = readSecrets();
    Serial.println("read secrets");
    bool circleValues = true;
//    rotaryEncoder.setBoundaries(0, secrets.size(), circleValues);
    std::vector<std::string> menuItems;
    for (auto item : secrets) {
      sprintf(buff, "%s(%s)", item.issuer.c_str(), item.username.c_str());
      Serial.println(buff);
      menuItems.push_back(buff);
    }
//    for (uint8_t ch = 'a'; ch <= 'z'; ++ch) {
//      menuItems.push_back(std::string(1, ch));
//    }
//    for (uint8_t ch = '0'; ch <= '9'; ++ch) {
//      menuItems.push_back(std::string(1, ch));
//    }
//    std::vector<std::string> puncts = {
//      "_","+","-","=","!","@","#","$","%","^","&","*","(",")",":",";","\"","'","<",">","?",",",".","/","~","`"
//    };
//    menuItems.insert(menuItems.end(), puncts.begin(), puncts.end());
    mainMenu = new Menu(menuItems, tft);
    rotaryEncoder.setBoundaries(0, menuItems.size(), circleValues);

    tft.println("Connecting to WiFi");
//    Serial.println("about to connect to wifi");
    ESP_WiFiManager ESP_wifiManager("TOTPAP");
    ESP_wifiManager.autoConnect("TOTPAP");
    lastInputTime = millis();
    pinMode(34, ANALOG);
    Serial.println("connected wifi");
    tft.fillScreen(TFT_BLACK);
}

void loop()
{
  if (lastBatteryReading < 3200.0 && millis() - lastInputTime >= SLEEP_TIME) {
    Serial.println("Sleep");
    digitalWrite(TFT_BL, LOW);
    tft.writecommand(TFT_DISPOFF);
    tft.writecommand(TFT_SLPIN);
    //esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    rtc_gpio_pullup_en(GPIO_NUM_25);
    rtc_gpio_pullup_en(GPIO_NUM_33);
    rtc_gpio_pulldown_en(GPIO_NUM_32);
    Serial.print("gpio25 "); Serial.println(digitalRead(25));
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_25, 0);
    esp_deep_sleep_start();
  } else if (millis() - lastReadTime >= BATTERY_READ_TIME) {
    uint16_t battLevel = analogRead(34);
    lastBatteryReading = ((double)battLevel) * 2.0 * 2450.0 / 4096.0;
    //sprintf(buff, "Battery volts: %lf", lastBatteryReading);
    //Serial.println(buff);
    //tft.drawString(buff, 0, tft.height() - tft.fontHeight());
    lastReadTime = millis();
    if (lastBatteryReading >= 3200.0) {
      lastInputTime = millis();
    }
  }
  mainMenu->loop();
  rotary_loop();
}
