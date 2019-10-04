#include "asyncpinghelper.h"

#define PRINTPORT Serial
#define DEBUGPORT Serial

// #define RELEASE

#define PRINT(fmt, ...)                          \
    {                                            \
        static const char pfmt[] PROGMEM = fmt;  \
        PRINTPORT.printf_P(pfmt, ##__VA_ARGS__); \
    }

#ifndef RELEASE
#define DEBUGLOG(fmt, ...)                       \
    {                                            \
        static const char pfmt[] PROGMEM = fmt;  \
        DEBUGPORT.printf_P(pfmt, ##__VA_ARGS__); \
    }
#else
#define DEBUGLOG(...)
#endif


const char Disconnected_Str[] PROGMEM = "Disconnected";
const char Connected_Str[] PROGMEM = "Connected";

const char *const internetstatus_P[] PROGMEM =
    {
        Disconnected_Str,
        Connected_Str};

Ticker timer;

AsyncPing Pings[1];
IPAddress addrs[1];

// const char *ips[] = {NULL, "google.com", "8.8.8.8"};

const char *ips[] = {"8.8.8.8"};

bool internet = false;

void ping()
{
    for (int i = 0; i < 1; i++)
    {
        DEBUGLOG("\nstarted ping to %s:\n", addrs[i].toString().c_str());
        Pings[i].begin(addrs[i]);
    }
}

void PingSetup()
{
    for (int i = 0; i < 1; i++)
    {
        if (ips[i])
        {
            if (!WiFi.hostByName(ips[i], addrs[i]))
                addrs[i].fromString(ips[i]);
        }
        else
            addrs[i] = WiFi.gatewayIP();

        Pings[i].on(true, [](const AsyncPingResponse &response) {
            IPAddress addr(response.addr); //to prevent with no const toString() in 2.3.0
            if (response.answer)
            {
                DEBUGLOG("%d bytes from %s: icmp_seq=%d ttl=%d time=%d ms\n", response.size, addr.toString().c_str(), response.icmp_seq, response.ttl, response.time);
            }
            else
            {
                DEBUGLOG("no answer yet for %s icmp_seq=%d\n", addr.toString().c_str(), response.icmp_seq);
            }
            return false; //do not stop
        });

        Pings[i].on(false, [](const AsyncPingResponse &response) {
            IPAddress addr(response.addr); //to prevent with no const toString() in 2.3.0
            DEBUGLOG("total answer from %s sent %d received %d time %d ms\n\n", addr.toString().c_str(), response.total_sent, response.total_recv, response.total_time);

            if (response.total_sent == response.total_recv)
                internet = true;
            else
                internet = false;

            if (response.mac)
            {
                DEBUGLOG("detected eth address " MACSTR "\n", MAC2STR(response.mac->addr));
            }
            return true;
        });
    }

    ping();
    timer.attach(10, ping);
}