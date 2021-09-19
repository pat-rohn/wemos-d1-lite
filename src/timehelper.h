#pragma once
#include <Arduino.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

class CTimeHelper
{
public:
    CTimeHelper();
    bool initTime();
    String getTimestamp();
    String getTimestampESP8266();

    String fillUpZeros(int number);

private:
    NTPClient timeClient;
};