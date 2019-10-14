#pragma once
#include <FS.h>   // Include the SPIFFS library

class SaveDataClass
{
private:
  const char* savefilePath = "/settings.dat";
  const int saveSize = 9;
  uint8_t* savedata = new uint8_t[saveSize];

public:
  SaveDataClass(void) {}

  void load();

  void save();

  uint8_t read(int adress);

  void write(int adress, uint8_t value);
  
  void commit();
};

extern SaveDataClass SaveData;