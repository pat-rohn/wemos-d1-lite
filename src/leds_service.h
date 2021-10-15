#pragma once
#include <Arduino.h>

#include "WiFi.h"
#include <HTTPClient.h>


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
