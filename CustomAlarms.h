#pragma once

// to use:
// 1. include this header
// 2. call startTimer at the end of setup
// 3. call updateTimer at the start of update loop

#include <ezTime.h>
Timezone homeTZ;

typedef void(*Event)();

#define INVALID_EVENT_ID 0
uint8_t turnOn = INVALID_EVENT_ID, turnOff = INVALID_EVENT_ID;
void rescheduleAlarm(uint8_t& id, uint8_t time, Event call, bool today);

void updateTimer();
void rescheduleAlarms();

//  to resolve the circular header dependencies
void setPower(uint8_t value);

void eventPowerOn() {
  setPower(1);
  uint8_t on = SaveData.read(6);
  rescheduleAlarm(turnOn, on, eventPowerOn, false);
}

void eventPowerOff()
{
  setPower(0);
  uint8_t off = SaveData.read(7);
  rescheduleAlarm(turnOff, off, eventPowerOff, false);
}

void startTimer(String ntpName)
{
  Serial.println("CustomAlarms started");

  setServer(ntpName);

  Serial.println("pre-sync UTC: " + UTC.dateTime());
  homeTZ.setPosix("CET-1CEST,M3.5.0/1,m10.5.0/1");
  Serial.println("pre-sync Home: " + homeTZ.dateTime());

  waitForSync();
  events();
  Serial.println("UTC: " + UTC.dateTime());
  Serial.println("Home: " + homeTZ.dateTime());
  homeTZ.setDefault();

  rescheduleAlarms();

  setInterval(360);
  setDebug(DEBUG);
}

inline void updateTimer()
{
  events();
}

void rescheduleAlarms() {
  uint8_t on = SaveData.read(6);
  uint8_t off = SaveData.read(7);

  Serial.println("Scheduling Alarm: Turn On");
  rescheduleAlarm(turnOn, on, eventPowerOn, true);
  Serial.println("Scheduling Alarm: Turn Off");
  rescheduleAlarm(turnOff, off, eventPowerOff, true);
}

void rescheduleAlarm(uint8_t& id, uint8_t time, Event call, bool today) {
  Serial.print(" rescheduleAlarm raw: ");
  Serial.println(time);
  if (id != INVALID_EVENT_ID) {
    deleteEvent(id);
    id = INVALID_EVENT_ID;
  }
  if ((time & 0b10000000) == 0) {
    // Hour: 0 - 24               5 bits (range 32)
    uint8_t h = (time >> 2) & 0b00011111;
    // Minute : 0 - 60            2 bits, (range 4) quarters, 0=0, 1=15, 2=30, 3=45
    uint8_t q = time & 0b00000011;
    if (h > 24)
    {
      Serial.println(" error");
      return;
    }

    time_t timeNow = homeTZ.now();
    tmElements_t tm;
    breakTime(timeNow, tm);
	if (!today || tm.Hour > h)
	{
		tm.Day++;
	}
    tm.Hour = h;
    tm.Minute = 15 * q;

    id = setEvent(call, makeTime(tm));
  }
}