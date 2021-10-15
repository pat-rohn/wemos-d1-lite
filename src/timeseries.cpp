#ifdef MY_ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#endif

#ifdef MY_ESP32
#include "WiFi.h"
#include <HTTPClient.h>
#endif

#ifdef MY_M5STACKCORE2
#include <HTTPClient.h>
#include "lwip/apps/sntp.h"
#endif

#include <ArduinoJson.h>
#include "timeseries.h"

void CTimeseriesData::addValue(const double &value, String timestamp)
{
    if (!timestamp.isEmpty())
    {
        m_DataSeries.emplace_back(DataPoint(timestamp, value));
    }
}

CTimeseries::CTimeseries(const char *timeseriesAddress, const char *port)
{
    m_ServerAddress = "http://";
    m_ServerAddress += timeseriesAddress;
    m_ServerAddress += ":";
    m_ServerAddress += port;
    m_TimeHelper = CTimeHelper();
}

void CTimeseries::addValue(const String &name, const double &value)
{
    if (m_Data.find(name) == m_Data.end())
    {
        m_Data.insert(std::pair<String, CTimeseriesData>(name, CTimeseriesData(name)));
    }
    m_Data.at(name).addValue(value, m_TimeHelper.getTimestamp());
}

std::map<String, CTimeseriesData> CTimeseries::getValues()
{
    return m_Data;
}

bool CTimeseries::sendData()
{
    //https://arduinojson.org/v6/assistant/
    //https://arduinojson.org/v6/how-to/determine-the-capacity-of-the-jsondocument/

    DynamicJsonDocument doc(10000); // uses heap because it's too much data for stack
    for (auto const &ts : m_Data)
    {
        JsonObject tsEntry = doc.createNestedObject();
        tsEntry["Tag"] = ts.first;
        JsonArray tsValuesTS = tsEntry.createNestedArray("Timestamps");
        JsonArray tsValuesV = tsEntry.createNestedArray("Values");
        for (auto val : ts.second.m_DataSeries)
        {
            tsValuesTS.add(val.Timestamp);
            tsValuesV.add(String(val.Value));
        }
    }
    Serial.println(doc.as<String>());
    if (postData(doc.as<String>(), "/timeseries/save"))
    {
        m_Data.clear();
        return true;
    }
    return false;
}

bool CTimeseries::postData(const String &root, const String &url)
{
    WiFiClient client = WiFiClient();
    HTTPClient http;
    String serverPath = m_ServerAddress;
    serverPath += url;
    Serial.println(serverPath);

    http.begin(client, serverPath.c_str());
    Serial.println("Post data");
    int httpResponseCode = http.POST(root.c_str());

    if (httpResponseCode > 0)
    {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
        http.end();
        return true;
    }
    else
    {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
        http.end();
        return false;
    }
    // Free resources
    return true;
}
