#include <Arduino.h>
#include <time.h>
// #include <ESP8266WiFi.h>
#include "asyncserver.h"
#include "timehelper.h"
#include "rtchelper.h"
// #include "EspGoodies.h"
#include "asyncpinghelper.h"
#include <TZ.h>

#define MYTZ TZ_Asia_Jakarta


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

bool tick200ms = false;
bool tick1000ms = false;
bool tick3000ms = false;
bool state500ms = false;
bool state1000ms = false;
bool state250ms = false;
bool waitingForInternet = true;

uint32_t now_ms, now_us;
timeval tv;
timespec tp;
timeval cbtime; // time set in callback

bool timeSetFlag = false;
bool timeSetOnce = false;
bool syncTimeFromRtcFlag = false;
bool syncByRtcFlag = false;
bool syncByNtpFlag = false;
bool internetAccess = true;
bool y2k38mode = false;

uint32_t now;
uint32_t localTime;
uint32_t utcTime;
uint32_t uptime;
// uint32_t lastSync;          ///< Stored time of last successful sync
uint32_t lastSyncByNtp = 0; ///< Stored time of last successful sync
uint32_t lastSyncByRtc = 0; ///< Stored time of last successful sync
uint32_t _firstSync;        ///< Stored time of first successful sync after boot
uint32_t _lastBoot;
uint32_t nextSync;
;

uint16_t yearLocal;
uint8_t monthLocal;
uint8_t mdayLocal;
uint8_t wdayLocal;
uint8_t hourLocal;
uint8_t minLocal;
uint8_t secLocal;

uint16_t syncInterval;      ///< Interval to set periodic time sync
uint16_t shortSyncInterval; ///< Interval to set periodic time sync until first synchronization.
uint16_t longSyncInterval;  ///< Interval to set periodic time sync

Ticker tick200msTimer;
Ticker state250msTimer;
Ticker state500msTimer;
Ticker syncTimeFromRtcTicker;
Ticker waitingForInternetConnectedTimer;

strConfigTime _configTime;
strTimeSource timeSource;

float TimezoneFloat()
{
  time_t rawtime;
  char buffer[6];

  if (!y2k38mode)
  {
    rawtime = time(nullptr);
    // Serial.println("y2k38mode NO");
  }
  else if (y2k38mode)
  {
    rawtime = time(nullptr) + 2145916800 + 3133696;
    // Serial.println("y2k38mode YES");
  }

  strftime(buffer, 6, "%z", localtime(&rawtime));
  // Serial.println(buffer);

  char bufTzHour[4];
  strlcpy(bufTzHour, buffer, sizeof(bufTzHour));
  // Serial.println(bufTzHour);
  int8_t hour = atoi(bufTzHour);

  char bufTzMin[4];
  bufTzMin[0] = buffer[0]; // sign
  bufTzMin[1] = buffer[3];
  bufTzMin[2] = buffer[4];
  bufTzMin[3] = 0;
  float min = atoi(bufTzMin) / 60.0;

  float TZ_FLOAT = hour + min;
  return TZ_FLOAT;
}

int32_t TimezoneMinutes()
{
  return TimezoneFloat() * 60;
}

int32_t TimezoneSeconds()
{
  return TimezoneMinutes() * 60;
}

char *getDateStr(uint32_t moment)
{
  // output format: Fri 08 Jun 1979
  static char buf[16];

  RtcDateTime dt;
  dt.InitWithEpoch32Time(moment);

  snprintf_P(
      buf,
      (sizeof(buf)),
      PSTR("%s %d %s %d"),
      dayShortStr(dt.DayOfWeek()),
      dt.Day(),
      monthShortStr(dt.Month()),
      dt.Year());

  return buf;
}

char *getTimeStr(uint32_t moment)
{
  // output format: 15:57:96
  static char buf[9];

  RtcDateTime dt;
  dt.InitWithEpoch32Time(moment);

  snprintf_P(
      buf,
      (sizeof(buf)),
      PSTR("%02d:%02d:%02d"),
      dt.Hour(),
      dt.Minute(),
      dt.Second());

  return buf;
}

char *getDateTimeStr(uint32_t moment)
{
  // output: Tue Jul 24 2018 16:46:44 GMT
  static char buf[29];

  RtcDateTime dt;
  dt.InitWithEpoch32Time(moment);

  snprintf_P(
      buf,
      (sizeof(buf)),
      PSTR("%s %s %2d %02d:%02d:%02d %d"),
      dayShortStr(dt.DayOfWeek()),
      monthShortStr(dt.Month()),
      dt.Day(),
      dt.Hour(),
      dt.Minute(),
      dt.Second(),
      dt.Year());

  return buf;
}

char *getLastBootStr()
{
  // output: Tue Jul 24 2018 16:46:44 GMT
  static char buf[29];

  uint32_t lastBoot = now - uptime;

  RtcDateTime dt;
  dt.InitWithEpoch32Time(lastBoot);

  snprintf_P(
      buf,
      (sizeof(buf)),
      PSTR("%s %d %s %d %02d:%02d:%02d GMT"),
      dayShortStr(dt.DayOfWeek()),
      dt.Day(),
      monthShortStr(dt.Month()),
      dt.Year(),
      dt.Hour(),
      dt.Minute(),
      dt.Second());

  return buf;
}

char *getUptimeStr()
{
  // format: 365000 days 23:47:22

  uint16_t days;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;

  days = elapsedDays(uptime);
  hours = numberOfHours(uptime);
  minutes = numberOfMinutes(uptime);
  seconds = numberOfSeconds(uptime);

  static char buf[21];
  snprintf(buf, sizeof(buf), "%d days %02d:%02d:%02d", days, hours, minutes, seconds);

  return buf;
}

char *getLastSyncStr(uint32_t moment)
{
  // format: 365000 days 23 hrs ago

  uint32_t diff = now - moment;

  uint16_t days;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;

  days = elapsedDays(diff);
  hours = numberOfHours(diff);
  minutes = numberOfMinutes(diff);
  seconds = numberOfSeconds(diff);

  static char buf[23];
  if (days > 0)
  {
    snprintf(buf, sizeof(buf), "%u day %d hr ago", days, hours);
  }
  else if (hours > 0)
  {
    snprintf(buf, sizeof(buf), "%d hr %d min ago", hours, minutes);
  }
  else if (minutes > 0)
  {
    snprintf(buf, sizeof(buf), "%d min ago", minutes);
  }
  else
  {
    snprintf(buf, sizeof(buf), "%d sec ago", seconds);
  }

  return buf;
}

char *getNextSyncStr()
{
  // format: 365000 days 23:47:22

  // // time_t _syncInterval = 3601;
  // uint32_t _syncInterval = _configTime.syncinterval;

  // time_t diff = (lastSync + _syncInterval) - now;

  // uint16_t days;
  // uint8_t hours;
  // uint8_t minutes;
  // uint8_t seconds;

  // days = elapsedDays(diff);
  // hours = numberOfHours(diff);
  // minutes = numberOfMinutes(diff);
  // seconds = numberOfSeconds(diff);

  // static char buf[21];
  // snprintf(buf, sizeof(buf), "%d days %02d:%02d:%02d", days, hours, minutes, seconds);

  // return buf;

  // unsigned long nextsync;

  // nextsync = (_configTime.lastsync + _configTime.syncinterval) - utcTime;

  time_t diff = nextSync - now;

  uint16_t days;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;

  days = elapsedDays(diff);
  hours = numberOfHours(diff);
  minutes = numberOfMinutes(diff);
  seconds = numberOfSeconds(diff);

  static char buf[30];
  snprintf_P(buf, sizeof(buf), PSTR("%d days %02d:%02d:%02d"), days, hours, minutes, seconds);
  // snprintf_P(buf, sizeof(buf), PSTR("xx days %02d:%02d:%02d"), hours, minutes, seconds);

  return buf;
}

char *GetRtcDateTimeStr(const RtcDateTime &dt)
{
  // output format: Sat Jul 21 2018 10:59:32 GMT
  static char buf[29];

  snprintf_P(buf,
             //countof(datestring),
             (sizeof(buf) / sizeof(buf[0])),
             PSTR("%s %s %2d %02d:%02d:%02d %d"),

             dayShortStr(dt.DayOfWeek()),
             monthShortStr(dt.Month()),
             dt.Day(),
             dt.Hour(),
             dt.Minute(),
             dt.Second(),
             dt.Year());

  return buf;
}

void FlipWaitingForInternet()
{
  waitingForInternet = false;
}

void Tick200ms()
{
  //  static boolean state250ms;
  tick200ms = true;
}

void FlipState250ms()
{
  //  static boolean state250ms;
  state250ms = !state250ms;
}

void FlipState500ms()
{
  state500ms = !state500ms;
}

void RaiseSyncTimeFromRtcFlag()
{
  syncTimeFromRtcFlag = true;
}

#define PTM(w)              \
  Serial.print(":" #w "="); \
  Serial.print(tm->tm_##w);

void printTm(const char *what, const tm *tm)
{
  Serial.print(what);
  PTM(isdst);
  PTM(yday);
  PTM(wday);
  PTM(year);
  PTM(mon);
  PTM(mday);
  PTM(hour);
  PTM(min);
  PTM(sec);
}

void time_is_set()
{
  if (waitingForInternetConnectedTimer.active())
    waitingForInternetConnectedTimer.detach();

  timeSetFlag = true;
  timeSetOnce = true;
  y2k38mode = false;

  gettimeofday(&cbtime, NULL);

  DEBUGLOG("time(nullptr): %u, (uint32_t)tv.tv_sec: %u\r\n\r\n", time(nullptr), (uint32_t)cbtime.tv_sec);

  gettimeofday(&tv, nullptr);
  // clock_gettime(0, &tp);

  now = time(nullptr);

  if (time(nullptr) < 0)
  {
    y2k38mode = true;
  }

  localTime = now + TimezoneSeconds();
  uptime = tp.tv_sec;
  now_ms = millis();
  now_us = micros();

  RtcDateTime dt;
  dt.InitWithEpoch32Time(localTime);
  yearLocal = dt.Year();
  monthLocal = dt.Month();
  mdayLocal = dt.Day();
  wdayLocal = dt.DayOfWeek();
  hourLocal = dt.Hour();
  minLocal = dt.Minute();
  secLocal = dt.Second();

  utcTime = localTime - TimezoneSeconds();

  // clock_gettime(0, &tp);

  // now = time(nullptr);
  // DEBUGLOG("time(nullptr): %u, (uint32_t)tv.tv_sec: %u\r\n\r\n", time(nullptr), (uint32_t)tv.tv_sec);

  // uint32_t t = time(nullptr);

  // nextSync = t + _configTime.syncinterval;

  _configTime.lastsync = utcTime;

  save_config_time();

  if (syncByRtcFlag)
  {
    PRINT("\r\n------------------ settimeofday() was called by Rtc ------------------\r\n\r\n");
    syncByRtcFlag = true;
    syncByNtpFlag = false;
    lastSyncByRtc = utcTime;
    nextSync = utcTime + _configTime.syncinterval;
  }
  else
  {
    PRINT("\r\n------------------ settimeofday() was called By Ntp ------------------\r\n\r\n");
    syncByNtpFlag = true;
    syncByRtcFlag = false;
    lastSyncByNtp = utcTime;

    Rtc.SetDateTime(utcTime);

    nextSync = utcTime + 3600;
  }
}

void TimeSetup()
{
  tick200msTimer.attach(0.2, Tick200ms);
  state250msTimer.attach(0.25, FlipState250ms);

  settimeofday_cb(time_is_set);

  // configTZ(TZ_Asia_Jakarta);
  // configTZ(_configLocation.timezonestring);
  // configTZ("WIB-7");

  // setenv("TZ", _configLocation.timezonestring, 1 /*overwrite*/);
  // tzset();

  // configTime(TZ_Asia_Jakarta, "id.pool.ntp.org");
  configTime(_configLocation.timezonestring, _configTime.ntpserver_0, _configTime.ntpserver_1, _configTime.ntpserver_2);
  yield();

  if (_configTime.enablentp)
  {
    configTime(_configLocation.timezonestring, _configTime.ntpserver_0, _configTime.ntpserver_1, _configTime.ntpserver_2);
    yield();
  }

  // if (_configTime.enablertc && (!internetAccess || lastSyncByNtp == 0 || WiFi.status() != WL_CONNECTED || !_configTime.enablentp))
  // {
  //   uint32_t rtc = get_time_from_rtc();

  //   if (rtc > _configTime.lastsync)
  //   {
  //     syncByRtcFlag = true;

  //     lastSyncByRtc = rtc;
  //     _configTime.lastsync = rtc;
  //     save_config_time();

  //     timeval tv = {rtc, 0};
  //     timezone tz = {0, 0};
  //     settimeofday(&tv, &tz);
  //   }
  //   else
  //   {
  //     // timestamp error
  //   }
  // }
}

void TimeLoop()
{

  static bool ntpEnabled = _configTime.enablentp;

  if (_configTime.enablentp != ntpEnabled)
  {
    ntpEnabled = _configTime.enablentp;

    if (ntpEnabled)
    {
      // configTime(0, 0, _configTime.ntpserver_0, _configTime.ntpserver_1, _configTime.ntpserver_2);
      nextSync = utcTime + 3600;
    }
    else
    {
      sntp_stop();
      nextSync = utcTime + _configTime.syncinterval;
    }    
  }
  
  if (_configTime.enablentp || _configTime.enablertc)
  {
    if (now >= nextSync)
    {
      if (_configTime.enablentp)
      {
        if (WiFi.getMode() == WIFI_STA)
        {
          if (WiFi.localIP())
          {
            if (!internet)
            {
              if (!waitingForInternet)
              {
                syncTimeFromRtcFlag = true;
              }
            }
          }
          else
          {
            if (!waitingForInternet)
            {
              syncTimeFromRtcFlag = true;
            }
          }
        }
      }
      else if (_configTime.enablertc)
      {
        syncTimeFromRtcFlag = true;
      }
    }
  }
  else
  {
    // error
    // no valid time source available
  }
  

  if (syncTimeFromRtcFlag)
  {
    syncTimeFromRtcFlag = false;

    rtcStatus = GetRtcStatus();

    if (rtcStatus == RTC_TIME_VALID)
    {
      uint32_t rtc = get_time_from_rtc();

      if (rtc > _configTime.lastsync)
      {
        syncByRtcFlag = true; // flag to indicate that time was synced using Rtc

        timeval tv = {rtc, 0};
        timezone tz = {0, 0};
        settimeofday(&tv, &tz);
      }
      else
      {
        // something has gone wrong
        nextSync = nextSync + 20; // fast sync if error; better display it in led matrix
      }
    }
    else
    {
      // rtc error
      // do not display time
    }
  }

  gettimeofday(&tv, nullptr);
  clock_gettime(0, &tp);
  // Serial.printf("timezone:  %s\n", getenv("TZ") ? : "(none)");

  now = time(nullptr);

  if (time(nullptr) < 0)
  {
    y2k38mode = true;
  }

  localTime = now + TimezoneSeconds();
  uptime = tp.tv_sec;
  now_ms = millis();
  now_us = micros();

  RtcDateTime dt;
  dt.InitWithEpoch32Time(localTime);
  yearLocal = dt.Year();
  monthLocal = dt.Month();
  mdayLocal = dt.Day();
  wdayLocal = dt.DayOfWeek();
  hourLocal = dt.Hour();
  minLocal = dt.Minute();
  secLocal = dt.Second();

  utcTime = localTime - TimezoneSeconds();

  // if (syncByNtpFlag)
  // {
  //   syncByNtpFlag = false;
  //   lastSyncByNtp = utcTime;

  //   syncTimeFromRtcTicker.detach();
  // }

  // localtime / gmtime every second change
  static uint32_t lastv = 0;
  if (lastv != now)
  {
    lastv = now;
    DEBUGLOGLN("tv.tv_sec:%li", tv.tv_sec);

    state500ms = true;
    state500msTimer.once(0.5, FlipState500ms);

    tick1000ms = true;

    if (!(now % 2))
    {
      // do something even
      state1000ms = true;
    }
    else
    {
      state1000ms = false;
    }

    static uint8_t counter3 = 0;
    counter3++;
    if (counter3 >= 3)
    {
      counter3 = 0;
      tick3000ms = true;
    }

#ifndef RELEASE
    time_t test;
    if (!y2k38mode)
    {
      test = time(nullptr);
    }
    else if (y2k38mode)
    {
      test = time(nullptr) + 2145916800 + 3133696;
    }

    Serial.println();
    printTm("gmtime   ", gmtime(&test));
    Serial.println();
    printTm("localtime", localtime(&test));
    Serial.println();

    // time from boot
    Serial.print("clock:");
    Serial.print((uint32_t)tp.tv_sec);
    Serial.print("/");
    Serial.print((uint32_t)tp.tv_nsec);
    Serial.print("ns");

    // time from boot
    Serial.print(" millis:");
    Serial.print(now_ms);
    Serial.print(" micros:");
    Serial.print(now_us);

    // EPOCH+tz+dst
    Serial.print(" gtod:");
    Serial.print((uint32_t)tv.tv_sec);
    Serial.print("/");
    Serial.print((uint32_t)tv.tv_usec);
    Serial.print("us");

    // EPOCH+tz+dst
    Serial.print(" time_t:");
    Serial.print(now);
    Serial.print(" time uint32_t:");
    Serial.println((uint32_t)now);

    // RtcDateTime timeToSetToRTC;
    // timeToSetToRTC.InitWithEpoch32Time(now);
    // Rtc.SetDateTime(timeToSetToRTC);

    // human readable
    // Printed format: Wed Oct 05 2011 16:48:00 GMT+0200 (CEST)
    char buf[60];
    // output: Wed Jan 01 1902 07:02:04 GMT+0700 (WIB)
    // strftime(buf, sizeof(buf), "%a %b %d %Y %X GMT", gmtime(&test));
    strftime(buf, sizeof(buf), "%c GMT", gmtime(&test));
    DEBUGLOGLN("NTP GMT   date/time: %s", buf);
    // strftime(buf, sizeof(buf), "%a %b %d %Y %X GMT%z (%Z)", localtime(&test));
    strftime(buf, sizeof(buf), "%c GMT%z (%Z)", localtime(&test));
    DEBUGLOGLN("NTP LOCAL date/time: %s", buf);

    RtcDateTime dt;
    dt.InitWithEpoch32Time(now);
    DEBUGLOGLN("NTP GMT   date/time: %s GMT", getDateTimeStr(now));
    dt.InitWithEpoch32Time(localTime);
    strftime(buf, sizeof(buf), "GMT%z (%Z)", localtime(&test));
    DEBUGLOGLN("NTP LOCAL date/time: %s %s", getDateTimeStr(localTime), buf);

    dt = Rtc.GetDateTime();
    DEBUGLOGLN("RTC GMT   date/time: %s GMT\r\n", GetRtcDateTimeStr(dt));
    
#endif
  }
}
