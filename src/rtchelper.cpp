#include "rtchelper.h"
#include <time.h>

#define PRINTPORT Serial
#define DEBUGPORT Serial

#define PRINT(fmt, ...)                      \
  {                                          \
    static const char pfmt[] PROGMEM = fmt;  \
    PRINTPORT.printf_P(pfmt, ##__VA_ARGS__); \
  }

#define RELEASE

#ifndef RELEASE
#define DEBUGLOG(fmt, ...)                   \
  {                                          \
    static const char pfmt[] PROGMEM = fmt;  \
    DEBUGPORT.printf_P(pfmt, ##__VA_ARGS__); \
  }
#define DEBUGLOGLN(fmt, ...)                 \
  {                                          \
    static const char pfmt[] PROGMEM = fmt;  \
    static const char rn[] PROGMEM = "\r\n"; \
    DEBUGPORT.printf_P(pfmt, ##__VA_ARGS__); \
    DEBUGPORT.printf_P(rn);                  \
  }
#else
#define DEBUGLOG(...)
#define DEBUGLOGLN(...)
#endif

RtcDS3231<TwoWire> Rtc(Wire);

// bool syncRtcEventTriggered; // True if a time even has been triggered
// bool syncSuccessByRtc;
uint8_t rtcStatus = -1;

float rtcTemperature;

char bufCommonRtc[30];

float GetRtcTemp()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  RtcTemperature temp = Rtc.GetTemperature();
  return temp.AsFloatDegC();
}

uint8_t GetRtcStatus()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
  // status = 0; RTC time is valid.
  // status = 1; RTC lost confidence in the DateTime!
  // status = 2; Actual clock is NOT running on the RTC

  if (Rtc.GetIsRunning())
  {
    if (Rtc.IsDateTimeValid())
    {
      return RTC_TIME_VALID;
    }
    else if (!Rtc.IsDateTimeValid())
    {
      return RTC_LOST_CONFIDENT;
    }
  }

  return CLOCK_NOT_RUNNING;
}

// void SetRtcTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
// {
//   DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

//   RtcDateTime dt(year, month, day, hour, minute, second);

//   // t in local
//   uint32_t t = dt.Epoch32Time();

//   // convert t to utc
//   t -= TimezoneSeconds();

//   // update struct using t utc
//   dt.InitWithEpoch32Time(t);

//   // finally, set rtc time
//   Rtc.SetDateTime(dt);
// }

void SetRtcTime(uint32_t moment)
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
  RtcDateTime timeToSetToRTC;
  timeToSetToRTC.InitWithEpoch32Time(moment);
  Rtc.SetDateTime(timeToSetToRTC);
}

uint32_t get_time_from_rtc()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  RtcDateTime dt = Rtc.GetDateTime();
  
  uint32_t t = dt.Epoch32Time();

  return t; // return 0 if unable to get the time
}

void RtcSetup()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  Rtc.Begin();

  uint32_t timeRtc = get_time_from_rtc();

  if (!Rtc.IsDateTimeValid())
  {
    // Common Causes:
    //    1) first time you ran and the device wasn't running yet
    //    2) the battery on the device is low or even missing

    // PRINT("RTC LOST CONFIDENCE, timestamp (UTC):%li, %s\r\n", timeRtc, getDateTimeStr(timeRtc));
    PRINT("RTC LOST CONFIDENCE, timestamp (UTC):%u\r\n", timeRtc);

    unsigned long currMillis = millis();
    while (!Rtc.IsDateTimeValid() && millis() - currMillis <= 5000)
    {
      delay(500);
      PRINT(">");
    }
    PRINT("\r\n");

    if (timeRtc < 1514764800)
    {
      PRINT("RTC is older than 1 January 2018 :-(\r\n");
    }
    else if (timeRtc > 1514764800)
    {
      PRINT("but strange.., RTC is seems valid, 1.e. later than 1 January 2018...\r\n");
      //  RtcDateTime timeToSetToRTC;
      //  timeToSetToRTC.InitWithEpoch32Time(timeRtc);
      //  Rtc.SetDateTime(timeToSetToRTC);
    }
  }
  else
  {
    // PRINT("RTC time is VALID, timestamp (UTC):%li, %s\r\n", timeRtc, GetRtcDateTimeStr(timeRtc));
    PRINT("RTC time is VALID, timestamp (UTC):%u\r\n", timeRtc);
  }

  if (!Rtc.GetIsRunning())
  {
    PRINT("RTC was not actively running, starting now.\r\n");
    Rtc.SetIsRunning(true);
  }

  // never assume the Rtc was last configured by you, so
  // just clear them to your needed state
  Rtc.Enable32kHzPin(true);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeClock);
  Rtc.SetSquareWavePinClockFrequency(DS3231SquareWaveClock_1Hz);

  // for testing purpose
  if (GetRtcStatus() == RTC_LOST_CONFIDENT)
  {
    uint32_t t = get_time_from_rtc();
    if (t > 1514764800 && t <= 4102444800)
    {
      DEBUGLOGLN("RTC_LOST_CONFIDENT but the timestamp is bigger than 1514764800 & less than 4102444800");
      DEBUGLOGLN("Clear RTC invalid flag.");
      RtcDateTime timeToSetToRTC;
      timeToSetToRTC.InitWithEpoch32Time(t);
      Rtc.SetDateTime(timeToSetToRTC);
    }
  }
}