#include "SaveData.h"

void SaveDataClass::load()
{
  if (!SPIFFS.exists(savefilePath)) 
  {
    return;
  }
  File file = SPIFFS.open(savefilePath, "r");
  file.read(savedata, saveSize);
  file.close();

  Serial.print("Loaded: ");
  for (int i = 0; i < saveSize; i++) {
    Serial.printf("%d,", savedata[i]);
  }
  Serial.println("");
}

void SaveDataClass::commit() {
  save();
}

void SaveDataClass::save() {
  File file = SPIFFS.open(savefilePath, "w");
  file.write(savedata, saveSize);
  file.close();
}

uint8_t SaveDataClass::read(int adress) {
  return savedata[adress];
}

void SaveDataClass::write(int adress, uint8_t value) {
  if (read(adress) == value)
  {
    return;
  }
  savedata[adress] = value;
}

SaveDataClass SaveData;