#include "sensors.h"
//#include <M5Core2.h>
#include <Adafruit_Sensor.h>

#include "DHT.h"
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BME280.h>
#include "Adafruit_CCS811.h"
//#include "Adafruit_SGP30.h"
#include <Adafruit_BMP280.h>

//#include <WEMOS_SHT3X.h>
// uint8_t DHTPin = D2;
uint8_t DHTPIN = 4;
#define DHTTYPE DHT22 //DHT11, DHT21, DHT22
DHT dht(DHTPIN, DHTTYPE);

std::map<String, SensorType> m_SensorTypes;

Adafruit_BME280 bme; // I2C

Adafruit_CCS811 ccs;
TwoWire MyWire = Wire;

Adafruit_BMP280 bmp = Adafruit_BMP280(&MyWire); // external i2c
Adafruit_Sensor *bmp_temp = bmp.getTemperatureSensor();
Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();
//SHT3X sht30(0x44);

//Adafruit_SGP30 sgp;

bool sensorsInit()
{
    Serial.println("Find and init sensors.");
    //MyWire.begin();
    pinMode(DHTPIN, INPUT);

    dht.begin();
    auto dhtData = getDHT22(true);
    if (dhtData[1].isValid)
    {
        return true;
    }
    MyWire.begin();

    for (int i = 0; i < 120; i++)
    {
        MyWire.beginTransmission(i);
        if (MyWire.endTransmission() == 0)
        {
            Serial.printf("%02X   |FIND", i);
            Serial.println(".");
        }
    }
    if (findAndInitSensors().empty())
    {

        MyWire.begin(32, 33);
        Serial.println("Change Wire since nothing found.");

        return (!findAndInitSensors().empty());
    }
    return false;
}

std::map<String, SensorType> findAndInitSensors()
{
    byte count = 0;
    MyWire.begin();
    for (byte i = 8; i < 120; i++)
    {
        MyWire.beginTransmission(i);
        if (MyWire.endTransmission() == 0)
        {
            Serial.print("Found address: ");
            Serial.print(i, DEC);
            Serial.print(" (0x");
            Serial.print(i, HEX);
            Serial.println(")");
            count++;
            delay(1);
            initSensor(i);
        }
    }

    Serial.println();
    return m_SensorTypes;
}

std::map<String, SensorData> getValues()
{
    std::map<String, SensorData> res;

    for (const auto &val : getDHT22())
    {
        if (val.isValid)
        {
            res[val.name] = val;
        }
    }

    return res;
}

std::array<SensorData, 3> getBME280()
{
    std::array<SensorData, 3> res;
    float temperature = bme.readTemperature();
    float pressure = bme.readPressure();
    float humidity = bme.readHumidity();

    if (isnan(temperature) || isnan(pressure) || isnan(humidity))
    {
        Serial.println("BME280 konnte nicht ausgelesen werden");
        //delay(10000);
        return res;
    }

    if (pressure < 100)
    {
        Serial.println("No BME Connection");
        delay(10000);
        return res;
    }
    return res;
}

std::array<SensorData, 3> getjmcu()
{
    std::array<SensorData, 3> sensorData;
    sensorData.fill(SensorData());

    if (ccs.available())
    {
        float temp = ccs.calculateTemperature();
        if (!ccs.readData())
        {

            sensorData[0].isValid = true;
            sensorData[0].value = ccs.geteCO2();
            sensorData[0].unit = "ppm";
            sensorData[0].name = "CO2";

            sensorData[1].isValid = true;
            sensorData[1].value = ccs.getTVOC();
            sensorData[1].unit = "ppb";
            sensorData[1].name = "TVOC";

            Serial.print("CO2: ");
            Serial.print(ccs.geteCO2());
            Serial.print("\nppm, TVOC: ");
            Serial.print(ccs.getTVOC());
            Serial.print("\nppb Temp:");
            Serial.println(temp);
        }
        else
        {
            Serial.println("jmcu ERROR!");

            return sensorData;
        }
    }
    return sensorData;
}

std::array<SensorData, 3> getDHT22(bool force /*= false*/)
{
    std::array<SensorData, 3> sensorData;
    sensorData.fill(SensorData());

    float h = dht.readHumidity(force);
    float t = dht.readTemperature(false, force);
    if (!isnan(t))
    {
        sensorData[0].isValid = true;
        sensorData[0].value = t;
        sensorData[0].unit = "*C";
        sensorData[0].name = "Temperature";
        Serial.print(sensorData[0].name);
        Serial.print(": ");
        Serial.print(sensorData[0].value);
        Serial.println(sensorData[0].unit);
    }
    else
    {
        Serial.print("DHT no valid result:");
        Serial.println(t);
    }
    if (!isnan(h))
    {
        sensorData[1].isValid = true;
        sensorData[1].value = h;
        sensorData[1].unit = "%";
        sensorData[1].name = "Humidity";

        Serial.print(sensorData[1].name);
        Serial.print(": ");
        Serial.print(sensorData[1].value);
        Serial.println(sensorData[1].unit);
    }

    return sensorData;
}

std::array<SensorData, 3> getEnv()
{
    std::array<SensorData, 3> sensorData;
    sensorData.fill(SensorData());
    if (!bmp.begin(0x76))
    {
        Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                         "try a different address!"));
        return sensorData;
    }
    sensors_event_t temp_event, pressure_event;
    bmp_temp->getEvent(&temp_event);
    bmp_pressure->getEvent(&pressure_event);
    sensorData[0].isValid = true;
    sensorData[0].value = temp_event.temperature;
    sensorData[0].unit = "*C";
    sensorData[0].name = "Temperature";
    sensorData[1].isValid = true;
    sensorData[1].value = pressure_event.pressure;
    sensorData[1].unit = "mbar";
    sensorData[1].name = "Pressure";

    unsigned int data[6];

    // Start I2C Transmission
    MyWire.beginTransmission(0x44);
    // Send measurement command
    MyWire.write(0x2C);
    MyWire.write(0x06);
    // Stop I2C transmission
    if (MyWire.endTransmission() != 0)
    {
        return sensorData;
    }

    delay(500);

    // Request 6 bytes of data
    MyWire.requestFrom(0x44, 6);

    // Read 6 bytes of data
    // cTemp msb, cTemp lsb, cTemp crc, humidity msb, humidity lsb, humidity crc
    for (int i = 0; i < 6; i++)
    {
        data[i] = MyWire.read();
    };

    delay(50);

    if (MyWire.available() != 0)
    {
        return sensorData;
    }

    // Convert the data
    //double cTemp = ((((data[0] * 256.0) + data[1]) * 175) / 65535.0) - 45;
    //fTemp = (cTemp * 1.8) + 32;
    double humidity = ((((data[3] * 256.0) + data[4]) * 100) / 65535.0);
    sensorData[2].isValid = true;
    sensorData[2].value = humidity;
    sensorData[2].unit = "%";
    sensorData[2].name = "Humidity";

    return sensorData;
}

void initSensor(uint address)
{
    Serial.print("Init Sensor: 0x");
    Serial.println(address);
    if (address == 0x76)
    {
        unsigned status;
        status = bme.begin(address, &MyWire);
        if (!status)
        {
            Serial.print("SensorID was: 0x");
            Serial.println(bme.sensorID(), 16);

            if (!bmp.begin(0x76))
            {
                Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                                 "try a different address!"));
                return;
            }
            bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                            Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                            Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                            Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                            Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
            bmp_temp->printSensorDetails();
            m_SensorTypes["bmp280"] = SensorType::bmp280;
        }
        else
        {
            m_SensorTypes["bme280"] = SensorType::bme280;
        }
    }
    else if (address == 0x5A)
    {
        Serial.println("Init Sensor cjmcu");
        if (!ccs.begin())
        {
            Serial.println("Failed to start sensor! Please check your wiring.");
            return;
        }

        m_SensorTypes["cjmcu"] = SensorType::cjmcu;
        //calibrate temperature sensor
        while (!ccs.available())
        {
        }
        float cTemp = ccs.calculateTemperature();
        ccs.setTempOffset(cTemp - 25.0);
    }
    else if (address == 0x44)
    {
        Serial.println("Sensor SHT30.");
        m_SensorTypes["sht30"] = SensorType::sht30;
    }
    else if (address == 0x58)
    {
        /*Serial.println("Sensor SGP30.");
        Serial.print("Init SGP30 CO2 / TVOC");
        m_SensorTypes["sgp30"] = SensorType::sgp30;
        if (!sgp.begin(&MyWire))
        {
            Serial.println("Init SGP30 Failed");
        };
        Serial.print(sgp.serialnumber[0], HEX);
        Serial.print(sgp.serialnumber[1], HEX);
        Serial.println(sgp.serialnumber[2], HEX);*/
    }
}
