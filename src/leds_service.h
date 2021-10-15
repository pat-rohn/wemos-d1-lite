
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
#include <M5Core2.h>
#include "WiFi.h"
#include <HTTPClient.h>
#endif

#include "ledstrip.h"

class CLEDService
{
public:
    CLEDService(uint8_t ledPin);
    ~CLEDService(){};

public:
    WiFiServer m_Server;
    String header;

    LedStrip m_LedStrip;
    void beginPixels();
    void beginServer();
    void listen();
    String getHomepage();
    String getHTTPOK();
    String getHTTPNotOK();
    void fancy();
    void showError();

private:
    unsigned long m_CurrentTime;
    unsigned long m_PreviousTime;
    unsigned long m_TimeoutTime;
};
