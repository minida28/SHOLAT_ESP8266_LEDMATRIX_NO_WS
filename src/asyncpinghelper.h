#ifndef asyncpinghelper_h
#define asyncpinghelper_h

#include <ESP8266WiFi.h>
#include "AsyncPing.h"
#include "Ticker.h"

// Ticker timer;

// AsyncPing Pings[3];
// IPAddress addrs[3];

// const char *ips[]={NULL,"google.com","8.8.8.8"};

// void ping();

const char pgm_internetstatus[] PROGMEM = "internetstatus";

extern const char *const internetstatus_P[];

extern bool internet;

void PingSetup();


#endif