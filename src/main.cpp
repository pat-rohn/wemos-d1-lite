
#include <array>
#include <map>

#ifdef MY_ESP8266
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#endif

#ifdef MY_ESP32
#include <Arduino.h>
#include "WiFi.h"
#include <HTTPClient.h>
#endif

#ifdef MY_M5STACKCORE2
#include <HTTPClient.h>
#include <M5Core2.h>
#include "lwip/apps/sntp.h"
#include <m5stackCore2.h>
#endif

#include "configuration.h" // TODO: Create this file, see README

#include "timeseries.h"

#include "sensors.h"
#include "leds_service.h"

CTimeseries timeseries = CTimeseries(timeseriesAddress, port);
CLEDService ledService = CLEDService(LED_PIN);

bool hasSensors = false;

IPAddress local_IP(192, 168, 4, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);
IPAddress gateway(192, 168, 4, 1);

bool isAccessPoint = false;

const char *ssidAP = "AI-Caramba";
const char *passwordAP = "ki-caramba";

uint32_t SENSOR_SCANRATE = 20 * 1000L;
unsigned long target_time = 0L;

SensorType sensorType = SensorType::unknown;

bool tryConnect(std::string ssid, std::string password)
{
  Serial.print("Try connecting to: ");
  Serial.print(ssid.c_str());
  Serial.println("");
  WiFi.begin(ssid.c_str(), password.c_str());

  int counter = 0;
  while (WiFi.status() != WL_CONNECTED && counter <= 15)
  {
#ifdef MY_M5TACKCORE2
    digitalWrite(LED_BUILTIN, 0x0);
    delay(250);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, 0x1);
#endif
    delay(350);
    counter++;
    if (counter >= 15)
    {
      return false;
    }
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  return true;
}

bool connectToWiFi()
{
  WiFi.disconnect();
  for (auto const &x : wifiData)
  {
    if (tryConnect(x.first, x.second))
    {
      return true;
    }
  }
  return false;
}

void startLedControl()
{
  ledService.beginPixels();
  ledService.beginServer();
  ledService.fancy();
}

#ifdef MY_M5STACKCORE2
#include "WiFi.h"
#include <HTTPClient.h>

#include <M5Core2.h>

#include <SPI.h>
#include "timeseries.h"

word ConvertRGB(byte R, byte G, byte B)
{
  return (((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3));
}

void header(const char *string, uint16_t color)
{
  M5.Lcd.fillScreen(color);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(TFT_WHITE, ConvertRGB(52, 168, 235));
  M5.Lcd.fillRect(0, 0, 320, 30, ConvertRGB(52, 168, 235));
  M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.drawString(string, 160, 3, 4);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setCursor(80, 40, 4);
}

void setupM5()
{

  M5.begin(true, false, true, false);

  SPI.begin(SCK, MISO, MOSI, -1);
  header("Envy Iron Mint", TFT_BLACK);
}

void updateDisplay(std::map<String, CTimeseriesData> values)
{
  int yOffset = 50;
  M5.Lcd.fillRect(0, yOffset, 320, 320, TFT_BLACK);
  int yOffsetGap = 30;

  for (auto const &ts : values)
  {
    Serial.print(ts.first);
    Serial.print(": ");
    Serial.println(ts.second.m_DataSeries[ts.second.m_DataSeries.size() - 1].Value);
    String data = ts.first;
    data += ": ";
    data += ts.second.m_DataSeries[ts.second.m_DataSeries.size() - 1].Value;
    M5.Lcd.drawString(data, 150, yOffset, 4);
    yOffset += yOffsetGap;
  }

  M5.update();
}
#endif

void setup()
{
  Serial.begin(115200);
  Serial.println("setup");

#ifdef MY_M5STACKCORE2
  setupM5();
#endif /* M5STACKCORE2 */
#ifndef MY_M5STACKCORE2
  pinMode(LED_BUILTIN, OUTPUT);
#endif

  if (connectToWiFi())
  {
    if (sensorsInit(DHT_PIN))
    {
      hasSensors = true;
      return;
    }
    hasSensors = false;
    startLedControl();
    return;
  }
  else
  {
    WiFi.disconnect();
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
    {
      ledService.showError();
      Serial.println("STA Failed to configure");
      setup();
    }
    Serial.print("Has access point:");
    Serial.println(WiFi.localIP());
    isAccessPoint = true;
    if (!WiFi.mode(WIFI_AP))
    {
      Serial.print("mode failed.");
    }
    if (!WiFi.softAP(ssidAP, passwordAP))
    {
      Serial.print("softAP failed.");
    }
  }
  startLedControl();
}

unsigned long lastUpdate = millis();
unsigned long counter = 0;

void measureAndSendSensorData()
{
  if (millis() > lastUpdate + scanrate)
  {
    lastUpdate = millis();
    auto values = getValues();
    std::vector<String> valueNames;
    std::vector<float> tsValues;
    std::map<String, SensorData>::iterator it;
    if (values.empty())
    {
      Serial.println("No Values");
    }
    for (it = values.begin(); it != values.end(); it++)
    {
      if (it->second.isValid)
      {
        String name = sensorID;
        timeseries.addValue(name + it->second.name, it->second.value);
      }
    }
    counter++;
    if (counter > buffersize)
    {
      if (WiFi.status() != WL_CONNECTED)
      {
        if (!connectToWiFi())
        {
          Serial.println("No WiFi Connection");
          return;
        }
      }

#ifndef MY_M5STACKCORE2
      digitalWrite(LED_BUILTIN, 0x0);
#endif

      timeseries.sendData();
      counter = 0;

#ifndef MY_M5STACKCORE2
      digitalWrite(LED_BUILTIN, 0x1);
#endif
    }
  }
}

void loop()
{
  if (isAccessPoint || !hasSensors)
  { // is LED control
    ledService.listen();
    return;
  }
  measureAndSendSensorData();
#ifdef MY_M5STACKCORE2
  updateDisplay(timeseries.getValues());
#endif /*M5STACKCORE2*/
}
