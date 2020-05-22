/**********************************************************************************************************
 *
 * YouTube channel statistics from API with WiFiManager and e-Paper display.
 *
 * Thank you Brian Lough @ https://github.com/witnessmenow/arduino-youtube-api.
 * We will ride eternal, shiny, and chrome!
 **********************************************************************************************************/

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <YoutubeApi.h>

// e-Paper module
#define ENABLE_GxEPD2_GFX 1

#include <GxEPD2_3C.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_EPD.h>
#include <GxEPD2_GFX.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include "gfx/input_black.h"
#include "gfx/input_red.h"

#include <FS.h>
#include <LittleFS.h>

#include <DNSServer.h> //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h> //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>      //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <Wire.h>

// Custom values such as API keys, passwords, and channel IDs
#include "secrets.h"

GxEPD2_3C<GxEPD2_290c, GxEPD2_290c::HEIGHT> display(GxEPD2_290c(/*CS=15*/ 15, /*DC=16*/ 16,
                                                                /*RST=2*/ 2, /*BUSY=5*/ 5));
const int redLed = 4;

const int buttonPin = 1;
const int statusLed = 0;

const char *ssid = "YouTube Stats";
const char *password = PORTAL_WIFI_PASSWORD;
const char *css = "<style>html{background-color:#000;color:#fff;}"
                  "a{text-decoration:none;color:#eef;}h1{margin: 1em 0 1em 0;}"
                  "h1:before{content:'\\1F484\\1F48B';}</style>";

char apiKey[45] = YT_API_KEY;
char channelId[30] = YT_CHANNEL_ID;
bool firstBoot = true;

YoutubeApi *api;
WiFiClientSecure client;

const unsigned long API_REQUEST_DELAY = 137 * 1000;
unsigned long nextApiCallTime = 0;
long subs = 0;
bool shouldSaveConfig = false;

/**
 * Initialize wifi, display, timers, and pins.
 */
// ----------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(redLed, OUTPUT);
  for (int i = 0; i < 10; i++) {
    digitalWrite(redLed, LOW);
    delay(100);
    digitalWrite(redLed, HIGH);
    delay(100);
  }
  digitalWrite(redLed, LOW);

  if (LittleFS.begin()) {
    Serial.println("LittleFS Initialize....ok");
  } else {
    Serial.println("LittleFS Initialization...failed");
  }

  pinMode(statusLed, OUTPUT);

  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN,
               LOW); // Turn the LED on (Note that LOW is the voltage level
  digitalWrite(redLed, LOW);
  digitalWrite(statusLed, HIGH);

  display.init(115200);
  drawImage();
  delay(1000);
  display.powerOff();

  loadConfig();
  loadSubs();

  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setMinimumSignalQuality(40);
  wifiManager.setCustomHeadElement(css);

  // Add an additional config on the WIFI manager webpage for the API Key and
  // Channel ID
  WiFiManagerParameter customApiKey("apiKey", "API Key", apiKey, 50);
  WiFiManagerParameter customChannelId("channelId", "Channel ID", channelId, 35);
  wifiManager.addParameter(&customApiKey);
  wifiManager.addParameter(&customChannelId);

  if (!wifiManager.autoConnect(ssid, password)) {
    Serial.println("failed to connect, hit timeout");
    delay(3000);
    // reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
    return;
  }

  strcpy(apiKey, customApiKey.getValue());
  strcpy(channelId, customChannelId.getValue());

  if (shouldSaveConfig) {
    saveConfig();
  }

  digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH

  // Force Config mode if there is no API key
  if (strcmp(apiKey, "") > 0) {
    Serial.println("API key found, initializing YT API");
    client.setInsecure();
    api = new YoutubeApi(apiKey, client);
    api->_debug = true;
  } else {
    Serial.println("Forcing Config Mode to get api key");
    forceConfigMode();
  }

  showSubs(subs);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
}

// -----------------------------------------------------------
void loop() {
  if (digitalRead(buttonPin) == LOW) {
    // forceConfigMode();
  }

  if (millis() > nextApiCallTime || firstBoot) {
    firstBoot = false;
    digitalWrite(statusLed, LOW);
    digitalWrite(redLed, HIGH);
    if (api->getChannelStatistics(channelId)) {
      Serial.println("----------Stats----------");
      Serial.printf("%17s: %ld\n", "Subscriber Count", api->channelStats.subscriberCount);
      Serial.printf("%17s: %ld\n", "View Count", api->channelStats.viewCount);
      Serial.printf("%17s: %ld\n", "Comment Count", api->channelStats.commentCount);
      Serial.printf("%17s: %ld\n", "Video Count", api->channelStats.videoCount);
      Serial.println();

      showSubs(api->channelStats.subscriberCount);
      if (api->channelStats.subscriberCount != subs) {
        subs = api->channelStats.subscriberCount;
        saveSubs();
      }
    } else {
      Serial.println("api->getChannelStatistics failed");
    }
    digitalWrite(statusLed, HIGH);
    digitalWrite(redLed, LOW);
    nextApiCallTime = millis() + API_REQUEST_DELAY;
  }
}

// -----------------------------------------------------------
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());

  Serial.println(myWiFiManager->getConfigPortalSSID());

  // show portal connect instructions
}

// callback notifying us of the need to save config
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// -----------------------------------------------------------
bool loadSubs() {
  File subsFile = LittleFS.open("/subs.txt", "r");
  if (!subsFile) {
    Serial.println("Failed to open subs file");
    return false;
  }
  size_t size = subsFile.size();
  if (size > 8) {
    Serial.println("subs file size is too large");
    subsFile.close();
    return false;
  }
  long ss = 0;
  ss = subsFile.parseInt();
  subsFile.close();

  if (ss > 0) {
    subs = ss;
    Serial.printf("Loaded %ld subscribers from filesystem\n", subs);
  } else {
    Serial.printf("No subscribers loaded from filesystem");
  }
  return true;
}
bool saveSubs() {
  File subsFile = LittleFS.open("/subs.txt", "w");
  if (!subsFile) {
    Serial.println("Failed to open subs file for writing");
    return false;
  }
  subsFile.print(subs);
  subsFile.close();
  return true;
}

// -----------------------------------------------------------
bool loadConfig() {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);

  StaticJsonDocument<200> doc;
  auto error = deserializeJson(doc, buf.get());
  if (error) {
    Serial.println(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    return false;
  }

  // strcpy(apiKey, doc["apiKey"]);
  strcpy(apiKey, YT_API_KEY);
  strcpy(channelId, doc["channelId"]);
  return true;
}

// -----------------------------------------------------------
bool saveConfig() {
  StaticJsonDocument<200> doc;
  doc["apiKey"] = apiKey;
  doc["channelId"] = channelId;

  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  serializeJson(doc, configFile);
  return true;
}

// -----------------------------------------------------------
void forceConfigMode() {
  Serial.println("forceConfigMode() called");
  delay(100);
  WiFi.disconnect();
  Serial.println("Disconnected WiFi and restarting");
  delay(500);
  ESP.restart();
  delay(5000);
}

// -----------------------------------------------------------

struct bitmap_pair {
  const unsigned char *black;
  const unsigned char *red;
};

void drawImage() {
  display.setRotation(3);
  display.setFullWindow();

  bitmap_pair bitmap_pairs[] = {{input_black, input_red}};

  Serial.print(display.epd2.WIDTH);
  Serial.print(" x ");
  Serial.println(display.epd2.HEIGHT);

  if (display.epd2.panel == GxEPD2::GDEW029Z10) {
    for (uint16_t i = 0; i < sizeof(bitmap_pairs) / sizeof(bitmap_pair); i++) {
      display.firstPage();
      do {
        display.fillScreen(GxEPD_WHITE);
        display.drawBitmap(0, 0, bitmap_pairs[i].black, display.epd2.HEIGHT, display.epd2.WIDTH,
                           GxEPD_BLACK);
        display.drawBitmap(0, 0, bitmap_pairs[i].red, display.epd2.HEIGHT, display.epd2.WIDTH,
                           GxEPD_RED);
        yield();
      } while (display.nextPage());
      delay(2000);
    }
  }
}

// ---------------------------------------------------------------------
void showSubs(unsigned long subscribers) {
  delay(10);
  yield();

  char buffer[64];
  int written = snprintf(buffer, 64, "%d", subscribers);

  display.setRotation(3); // was 1
  display.setFont(&FreeSansBold24pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(buffer, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  x += 50;
  // display.setFullWindow();
  display.setPartialWindow(128, 0, 296 - 128, 128);
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    yield();
    display.print(buffer);
    yield();
  } while (display.nextPage());
  display.powerOff();
}
