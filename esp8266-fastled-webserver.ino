/*
   ESP8266 FastLED WebServer: https://github.com/jasoncoon/esp8266-fastled-webserver
   Copyright (C) 2015-2018 Jason Coon

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
extern "C" {
#include "user_interface.h"
}

#include <FS.h>
#include <EEPROM.h>

#include "LED_Functions.h"
#include "Field.h"
#include "Fields.h"

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include "Webserver_Functions.h"

// ===== Settings =====

// uncomment to enable Access Point mode
// #define AP_MODE

// name of the device in the local network
const char * mDNS_Name = "lights";

#include "Secrets.h" // this file is intentionally not included in the sketch, so nobody accidentally commits their secret information.
// create a Secrets.h file with the following:

// AP mode password
// const char WiFiAPPSK[] = "your-password";

// Wi-Fi network to connect to (if not in AP mode)
// char* ssid = "your-ssid";
// char* password = "your-password";

#define FRAME_ONCE_EVERY_MS 1000 / FRAMES_PER_SECOND
bool isOff = false;

void setup() {
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  
  LEDSetup();

  Serial.begin(74880);
  delay(100);
  Serial.setDebugOutput(true);

  Serial.print(F("Port: ")); Serial.println(DATA_PIN);

  EEPROM.begin(512);
  loadSettings();

  Serial.println();
  Serial.print(F("Heap: ")); Serial.println(system_get_free_heap_size());
  Serial.print(F("Boot Vers: ")); Serial.println(system_get_boot_version());
  Serial.print(F("CPU: ")); Serial.println(system_get_cpu_freq());
  Serial.print(F("SDK: ")); Serial.println(system_get_sdk_version());
  Serial.print(F("Chip ID: ")); Serial.println(system_get_chip_id());
  Serial.print(F("Flash ID: ")); Serial.println(spi_flash_get_id());
  Serial.print(F("Flash Size: ")); Serial.println(ESP.getFlashChipRealSize());
  Serial.print(F("Vcc: ")); Serial.println(ESP.getVcc());
  Serial.println();

  SPIFFS.begin();
  {
    Serial.println("SPIFFS contents:");

    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), String(fileSize).c_str());
    }
    Serial.printf("\n");
  }

  //disabled due to https://github.com/jasoncoon/esp8266-fastled-webserver/issues/62
  //initializeWiFi();

#ifdef AP_MODE
  WiFi.mode(WIFI_AP);

  // Do a little work to get a unique-ish name. Append the
  // last two bytes of the MAC (HEX'd) to "Thing-":
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
    String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();
  String AP_NameString = "ESP8266 Thing " + macID;

  int apNameSize = AP_NameString.length() + 1;
  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i = 0; i < AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);

  WiFi.softAP(AP_NameChar, WiFiAPPSK);

  Serial.printf("Connect to Wi-Fi access point: %s\n", AP_NameChar);
  Serial.println("and open http://192.168.4.1 in your browser");
#else
  WiFi.mode(WIFI_STA);
  Serial.printf("Connecting to %s\n", ssid);
  if (String(WiFi.SSID()) != String(ssid)) {
    WiFi.begin(ssid, password);
  }
#endif // AP_MODE

  //  webSocketsServer.begin();
  //  webSocketsServer.onEvent(webSocketEvent);
  //  Serial.println("Web socket server started");

  if (!MDNS.begin(mDNS_Name)) { // Start the mDNS responder for esp8266.local
    Serial.println("Error setting up MDNS responder!");
  }
  
  setupWebServer();
}

void loop() {
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(65535));

  //  dnsServer.processNextRequest();
  //  webSocketsServer.loop();
  webServer.handleClient();
  yield();
  //  handleIrInput();

  if (power == 0) {
    if (!isOff)
    {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      isOff = true;
    }
    FastLED.delay(100); // sleep-ish?
    return;
  }
  else {
    isOff = false;
  }

  static bool hasConnected = false;
  EVERY_N_SECONDS(1) {
    if (WiFi.status() != WL_CONNECTED) {
      //      Serial.printf("Connecting to %s\n", ssid);
      hasConnected = false;
    }
    else if (!hasConnected) {
      WiFi.hostname(mDNS_Name);
      hasConnected = true;
      Serial.print("Connected! Open http://");
      Serial.print(mDNS_Name);
      Serial.print(" or http://");
      Serial.print(WiFi.localIP());
      Serial.println(" in your browser");
    }
  }

  // change to a new cpt-city gradient palette
  EVERY_N_SECONDS(secondsPerPalette) {
    gCurrentPaletteNumber = addmod8(gCurrentPaletteNumber, 1, gGradientPaletteCount);
    gTargetPalette = gGradientPalettes[gCurrentPaletteNumber];
  }

  EVERY_N_MILLISECONDS(40) {
    // slowly blend the current palette to the next
    nblendPaletteTowardPalette(gCurrentPalette, gTargetPalette, 8);
    gHue++;  // slowly cycle the "base color" through the rainbow
  }

  // keep the framerate modest, handles client sooner then a delay
  EVERY_N_MILLISECONDS(FRAME_ONCE_EVERY_MS) {
    // Call the current pattern function once, updating the 'leds' array
    patterns[currentPatternIndex].pattern();
    FastLED.show();
  }
}

void loadSettings()
{
  brightness = EEPROM.read(0);

  currentPatternIndex = EEPROM.read(1);
  if (currentPatternIndex < 0)
    currentPatternIndex = 0;
  else if (currentPatternIndex >= patternCount)
    currentPatternIndex = patternCount - 1;

  byte r = EEPROM.read(2);
  byte g = EEPROM.read(3);
  byte b = EEPROM.read(4);

  if (r == 0 && g == 0 && b == 0)
  {
  }
  else
  {
    solidColor = CRGB(r, g, b);
  }

  power = EEPROM.read(5);

  currentPaletteIndex = EEPROM.read(8);
  if (currentPaletteIndex < 0)
    currentPaletteIndex = 0;
  else if (currentPaletteIndex >= paletteCount)
    currentPaletteIndex = paletteCount - 1;
}
