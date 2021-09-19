#include "timehelper.h"

WiFiUDP ntpUDP;

CTimeHelper::CTimeHelper() : timeClient(NTPClient(ntpUDP))
{
    //initTime();
    //timeClient.begin();
    // todo: fix this
    Serial.print("CTimeHelper");
}

bool CTimeHelper::initTime()
{
    unsigned long endTime = millis() + 10000;
    Serial.print("Synchronization");
    timeClient.begin();
    while (millis() < endTime)
    {
        Serial.print(" .. ");
        if (timeClient.forceUpdate())
        {
            Serial.print("Success");
            break;
        }
        delay(600);
    }

    Serial.println("");
    return timeClient.update();
}

String CTimeHelper::getTimestamp()
{
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;

    time(&now);

    localtime_r(&now, &timeinfo);
    unsigned long millisec = millis();
    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%dT%H:%M:%S.", &timeinfo);
    String timestamp = strftime_buf;
    char buf[10];
    sprintf(buf, "%03lu", millisec % 1000);

    timestamp += buf;
    timestamp += "Z";

    //Serial.print("Time:");
    //Serial.println(timestamp);
    return timestamp;
}

String CTimeHelper::getTimestampESP8266()
{
    timeClient.update();
    if (timeClient.isTimeSet())
    {
        unsigned long epochTime = timeClient.getEpochTime();
        struct tm *ptm = gmtime((time_t *)&epochTime);
        String timestamp = "";
        timestamp += ptm->tm_year + 1900;
        timestamp += "-";
        timestamp += fillUpZeros(ptm->tm_mon + 1);
        timestamp += "-";
        timestamp += fillUpZeros(ptm->tm_mday);
        timestamp += "T";
        timestamp += fillUpZeros(ptm->tm_hour);
        timestamp += ":";
        timestamp += fillUpZeros(ptm->tm_min);
        timestamp += ":";
        timestamp += fillUpZeros(ptm->tm_sec);
        timestamp += ".0";
        return timestamp;
    }
    return "";
}

String CTimeHelper::fillUpZeros(int number)
{
    String numberStr = "";
    if (number < 10)
    {
        numberStr += 0;
    }
    numberStr += number;
    return numberStr;
}
