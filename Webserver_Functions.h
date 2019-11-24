#include <ESP8266WebServer.h>
#include <FS.h>

#include "Field.h"
#include "Fields.h"


ESP8266WebServer webServer(80);
ESP8266HTTPUpdateServer httpUpdateServer;

File fsUploadFile;


///////////////////////////////////////////////////////////////////////
/////////////////////////  Function Prototypes    ////////////////////
//////////////////////////////////////////////////////////////////////
void sendCors();
void remoteSwitch(char letter, uint8_t on);
void setupWebServer();
void sendInt(uint8_t);
void sendString(String);
void broadcastInt(String, uint8_t);
void broadcastString(String, String);
String formatBytes(size_t);
String getContentType(String);
bool handleFileRead(String);
void handleFileUpload();
void handleFileDelete();
void handleFileCreate();
void handleFileList();
void setPower(uint8_t);
void setAutoplay(uint8_t);
void setAutoplayDuration(uint8_t);
void setSolidColor(CRGB);
void setSolidColor(uint8_t, uint8_t, uint8_t);
void adjustPattern(bool);
void setPattern(uint8_t);
void setPatternName(String);
void setPalette(uint8_t);
void setPaletteName(String);
void adjustBrightness(bool);
void setBrightness(uint8_t);


///////////////////////////////////////////////////////////////////////
/////////////////////////  Function Definitions    ///////////////////
//////////////////////////////////////////////////////////////////////

void sendCors()
{
  if (webServer.hasHeader("origin")) {
    String originValue = webServer.header("origin");
    webServer.sendHeader("Access-Control-Allow-Origin", originValue);
    webServer.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept, Authorization");
    webServer.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    webServer.sendHeader("Access-Control-Max-Age", "600");
    webServer.sendHeader("Vary", "Origin");
  }
}

void sendInt(uint8_t value)
{
  sendString(String(value));
}

void sendString(String value)
{
  webServer.send(200, "text/plain", value);
}

void broadcastInt(String name, uint8_t value)
{
  String json = "{\"name\":\"" + name + "\",\"value\":" + String(value) + "}";
  sendString(json);
  //  webSocketsServer.broadcastTXT(json);
}

void broadcastString(String name, String value)
{
  String json = "{\"name\":\"" + name + "\",\"value\":\"" + String(value) + "\"}";
  sendString(json);
  //  webSocketsServer.broadcastTXT(json);
}

//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  }
  else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  }
  else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
  else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

String getContentType(String filename) {
  if (webServer.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) {
  sendCors();
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = webServer.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload() {
  sendCors();
  if (webServer.uri() != "/edit") return;
  HTTPUpload& upload = webServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    //Serial.print("handleFileUpload Data: "); Serial.println(upload.currentSize);
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile)
      fsUploadFile.close();
    Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
  }
}

void handleFileDelete() {
  sendCors();
  if (webServer.args() == 0) return webServer.send(500, "text/plain", "BAD ARGS");
  String path = webServer.arg(0);
  Serial.println("handleFileDelete: " + path);
  if (path == "/")
    return webServer.send(500, "text/plain", "BAD PATH");
  if (!SPIFFS.exists(path))
    return webServer.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  webServer.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate() {
  sendCors();
  if (webServer.args() == 0)
    return webServer.send(500, "text/plain", "BAD ARGS");
  String path = webServer.arg(0);
  Serial.println("handleFileCreate: " + path);
  if (path == "/")
    return webServer.send(500, "text/plain", "BAD PATH");
  if (SPIFFS.exists(path))
    return webServer.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if (file)
    file.close();
  else
    return webServer.send(500, "text/plain", "CREATE FAILED");
  webServer.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  sendCors();

  if (!webServer.hasArg("dir")) { webServer.send(500, "text/plain", "BAD ARGS"); return; }

  String path = webServer.arg("dir");
  Serial.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while (dir.next()) {
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }

  output += "]";
  webServer.send(200, "text/json", output);
}

void setPower(uint8_t value)
{
  power = value > 0;
  SaveData.write(5, power);
  SaveData.commit();
  broadcastInt("power", power);
}

void setTimerOn(uint8_t value)
{
  SaveData.write(6, value);
  SaveData.commit();

  Serial.println("Scheduling Alarm: Turn On");
  rescheduleAlarm(turnOn, value, eventPowerOn, true);
}

void setTimerOff(uint8_t value)
{
  SaveData.write(7, value);
  SaveData.commit();

  Serial.println("Scheduling Alarm: Turn Off");
  rescheduleAlarm(turnOff, value, eventPowerOff, true);
}

void setSolidColor(CRGB color)
{
  setSolidColor(color.r, color.g, color.b);
}

void setSolidColor(uint8_t r, uint8_t g, uint8_t b)
{
  solidColor = CRGB(r, g, b);

  SaveData.write(2, r);
  SaveData.write(3, g);
  SaveData.write(4, b);
  setPattern(patternCount - 1);

  broadcastString("color", String(solidColor.r) + "," + String(solidColor.g) + "," + String(solidColor.b));
}

// increase or decrease the current pattern number, and wrap around at the ends
void adjustPattern(bool up)
{
  if (up)
    currentPatternIndex++;
  else
    currentPatternIndex--;

  // wrap around at the ends
  if (currentPatternIndex < 0)
    currentPatternIndex = patternCount - 1;
  if (currentPatternIndex >= patternCount)
    currentPatternIndex = 0;

  SaveData.write(1, currentPatternIndex);
  SaveData.commit();

  broadcastInt("pattern", currentPatternIndex);
}

void setPattern(uint8_t value)
{
  if (value >= patternCount)
    value = patternCount - 1;

  currentPatternIndex = value;

  SaveData.write(1, currentPatternIndex);
  SaveData.commit();

  broadcastInt("pattern", currentPatternIndex);
}

void setPatternName(String name)
{
  for (uint8_t i = 0; i < patternCount; i++) {
    if (patterns[i].name == name) {
      setPattern(i);
      break;
    }
  }
}

void setPalette(uint8_t value)
{
  if (value >= paletteCount)
    value = paletteCount - 1;

  currentPaletteIndex = value;

  SaveData.write(8, currentPaletteIndex);
  SaveData.commit();

  twinkleFoxPalette = palettes[currentPaletteIndex];

  broadcastInt("palette", currentPaletteIndex);
}

void setPaletteName(String name)
{
  for (uint8_t i = 0; i < paletteCount; i++) {
    if (paletteNames[i] == name) {
      setPalette(i);
      break;
    }
  }
}

void adjustBrightness(bool up)
{
  if (up && brightnessIndex < brightnessCount - 1)
    brightnessIndex++;
  else if (!up && brightnessIndex > 0)
    brightnessIndex--;

  brightness = brightnessMap[brightnessIndex];

  FastLED.setBrightness(brightness);

  SaveData.write(0, brightness);
  SaveData.commit();

  broadcastInt("brightness", brightness);
}

void setBrightness(uint8_t value)
{
  if (value > 255)
    value = 255;
  else if (value < 0) value = 0;

  brightness = value;

  FastLED.setBrightness(brightness);

  SaveData.write(0, brightness);
  SaveData.commit();

  broadcastInt("brightness", brightness);
}

void setupWebServer()
{
  httpUpdateServer.setup(&webServer);

  webServer.on("/all", HTTP_GET, []() {
    sendCors();
    String json = getFieldsJson(fields, fieldCount);
    webServer.send(200, "text/json", json);
  });

  webServer.on("/fieldValue", HTTP_GET, []() {
    sendCors();
    String name = webServer.arg("name");
    String value = getFieldValue(name, fields, fieldCount);
    webServer.send(200, "text/json", value);
  });

  webServer.on("/fieldValue", HTTP_POST, []() {
    sendCors();
    String name = webServer.arg("name");
    String value = webServer.arg("value");
    String newValue = setFieldValue(name, value, fields, fieldCount);
    webServer.send(200, "text/json", newValue);
  });

  webServer.on("/power", HTTP_POST, []() {
    sendCors();
    String value = webServer.arg("value");
    setPower(value.toInt());
    sendInt(power);
  });

  webServer.on("/timeOn", HTTP_POST, []() {
    sendCors();
    String value = webServer.arg("value");
    setTimerOn(value.toInt());
    sendInt(SaveData.read(6));
  });

  webServer.on("/timeOff", HTTP_POST, []() {
    sendCors();
    String value = webServer.arg("value");
    setTimerOff(value.toInt());
    sendInt(SaveData.read(7));
  });

  webServer.on("/cooling", HTTP_POST, []() {
    sendCors();
    String value = webServer.arg("value");
    cooling = value.toInt();
    broadcastInt("cooling", cooling);
    sendInt(cooling);
  });

  webServer.on("/sparking", HTTP_POST, []() {
    sendCors();
    String value = webServer.arg("value");
    sparking = value.toInt();
    broadcastInt("sparking", sparking);
    sendInt(sparking);
  });

  webServer.on("/speed", HTTP_POST, []() {
    sendCors();
    String value = webServer.arg("value");
    speed = value.toInt();
    broadcastInt("speed", speed);
    sendInt(speed);
  });

  webServer.on("/twinkleSpeed", HTTP_POST, []() {
    sendCors();
    String value = webServer.arg("value");
    twinkleSpeed = value.toInt();
    if (twinkleSpeed < 0) twinkleSpeed = 0;
    else if (twinkleSpeed > 8) twinkleSpeed = 8;
    broadcastInt("twinkleSpeed", twinkleSpeed);
    sendInt(twinkleSpeed);
  });

  webServer.on("/twinkleDensity", HTTP_POST, []() {
    sendCors();
    String value = webServer.arg("value");
    twinkleDensity = value.toInt();
    if (twinkleDensity < 0) twinkleDensity = 0;
    else if (twinkleDensity > 8) twinkleDensity = 8;
    broadcastInt("twinkleDensity", twinkleDensity);
    sendInt(twinkleDensity);
  });

  webServer.on("/solidColor", HTTP_POST, []() {
    sendCors();
    String r = webServer.arg("r");
    String g = webServer.arg("g");
    String b = webServer.arg("b");
    setSolidColor(r.toInt(), g.toInt(), b.toInt());
    sendString(String(solidColor.r) + "," + String(solidColor.g) + "," + String(solidColor.b));
  });

  webServer.on("/pattern", HTTP_POST, []() {
    sendCors();
    String value = webServer.arg("value");
    setPattern(value.toInt());
    twinkleFoxPalette = palettes[currentPaletteIndex];
    sendInt(currentPatternIndex);
  });

  webServer.on("/patternName", HTTP_POST, []() {
    sendCors();
    String value = webServer.arg("value");
    setPatternName(value);
    sendInt(currentPatternIndex);
  });

  webServer.on("/palette", HTTP_POST, []() {
    sendCors();
    String value = webServer.arg("value");
    setPalette(value.toInt());
    sendInt(currentPaletteIndex);
  });

  webServer.on("/paletteName", HTTP_POST, []() {
    sendCors();
    String value = webServer.arg("value");
    setPaletteName(value);
    sendInt(currentPaletteIndex);
  });

  webServer.on("/brightness", HTTP_POST, []() {
    sendCors();
    String value = webServer.arg("value");
    setBrightness(value.toInt());
    sendInt(brightness);
  });

  webServer.on("/button", HTTP_POST, []() {
    sendCors();
    String value = webServer.arg("value");
    remoteSwitch(value.charAt(0), value.substring(1).toInt());
  });

  //list directory
  webServer.on("/list", HTTP_GET, handleFileList);
  //load editor
  webServer.on("/edit", HTTP_GET, []() {
    sendCors();
    if (!handleFileRead("/edit.htm")) webServer.send(404, "text/plain", "FileNotFound");
  });
  //create file
  webServer.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  webServer.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  webServer.on("/edit", HTTP_POST, []() {
    sendCors();
    webServer.send(200, "text/plain", "");
  }, handleFileUpload);

  webServer.serveStatic("/", SPIFFS, "/", "max-age=86400");

  // list of headers to be recorded
  const char* headerkeys[] = { "origin" };
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);

  // ask server to track these headers
  webServer.collectHeaders(headerkeys, headerkeyssize);

  webServer.begin();
  Serial.println("HTTP web server started");
}


void remoteSwitch(char letter, uint8_t on)
{
    Serial.print(letter);
  Serial.println(on > 0 ? " on" : " off");
}