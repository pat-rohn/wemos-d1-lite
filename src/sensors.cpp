#include "sensors.h"

#include <Adafruit_Sensor.h>

#include "DHT.h"
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BME280.h>
#include "Adafruit_CCS811.h"
//#include "Adafruit_SGP30.h"
#include <Adafruit_BMP280.h>

//#include <WEMOS_SHT3X.h>

std::vector<SensorType> m_SensorTypes;

Adafruit_BME280 bme; // I2C

Adafruit_CCS811 ccs;
TwoWire MyWire = Wire;

Adafruit_BMP280 bmp = Adafruit_BMP280(&MyWire);
Adafruit_Sensor *bmp_temp = bmp.getTemperatureSensor();
Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();
//SHT3X sht30(0x44);
DHTSensor *dhtSensor;

//Adafruit_SGP30 sgp;

bool sensorsInit(uint8_t dhtPin)
{
    Serial.println("Sensors init.");
    m_SensorTypes.clear();
    dhtSensor = new DHTSensor(dhtPin);

    bool hasDHT = dhtSensor->init();
    if (hasDHT)
    {
        m_SensorTypes.emplace_back(SensorType::dht22);
    }
    findAndInitSensors();
    if (m_SensorTypes.empty() && !hasDHT)
    {
        //MyWire.begin(32, 33);
        Serial.println("Change Wire since nothing found.");

        return false;
    }
    return true;
}

void findAndInitSensors()
{
    Serial.println("Find and init i2c sensors");
    byte count = 0;
#ifdef MY_M5STACKCORE2
    MyWire.begin(32, 33);
#else
    MyWire.begin();
#endif
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
            Serial.print(")");
            count++;
            delay(0.1);
            initI2CSensor(i);
        }
    }

    Serial.println();
}

std::map<String, SensorData> getValues()
{
    std::map<String, SensorData> res;

    if (std::find(m_SensorTypes.begin(), m_SensorTypes.end(), SensorType::dht22) != m_SensorTypes.end())
    {
        for (const auto &val : getDHT22())
        {
            if (val.isValid)
            {
                res[val.name] = val;
            }
        }
    }

    if (std::find(m_SensorTypes.begin(), m_SensorTypes.end(), SensorType::cjmcu) != m_SensorTypes.end())
    {
        for (const auto &val : getCjmcu())
        {
            if (val.isValid)
            {
                res[val.name] = val;
            }
        }
    }
    if (std::find(m_SensorTypes.begin(), m_SensorTypes.end(), SensorType::bme280) != m_SensorTypes.end())
    {
        for (const auto &val : getBME280())
        {
            if (val.isValid)
            {
                res[val.name] = val;
            }
        }
    }
    if (std::find(m_SensorTypes.begin(), m_SensorTypes.end(), SensorType::sht30) != m_SensorTypes.end())
    {
        for (const auto &val : getSht30())
        {
            if (val.isValid)
            {
                res[val.name] = val;
            }
        }
    }

    return res;
}

std::array<SensorData, 3> getDHT22()
{
    std::array<SensorData, 3> sensorData;
    sensorData.fill(SensorData());
    std::pair<float, float> values = dhtSensor->read();

    if (!isnan(values.first))
    {
        sensorData[0].isValid = true;
        sensorData[0].value = values.first;
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
        Serial.println(values.first);
    }
    if (!isnan(values.second))
    {
        sensorData[1].isValid = true;
        sensorData[1].value = values.second;
        sensorData[1].unit = "%";
        sensorData[1].name = "Humidity";

        Serial.print(sensorData[1].name);
        Serial.print(": ");
        Serial.print(sensorData[1].value);
        Serial.println(sensorData[1].unit);
    }

    return sensorData;
}

std::array<SensorData, 3> getBME280()
{
    std::array<SensorData, 3> sensorData;
    float temperature = bme.readTemperature();
    float pressure = bme.readPressure();
    float humidity = bme.readHumidity();

    if (isnan(temperature) || isnan(pressure) || isnan(humidity))
    {
        Serial.println("BME280 konnte nicht ausgelesen werden");
        return sensorData;
    }

    if (pressure < 100)
    {
        Serial.println("No BME Connection");
        return sensorData;
    }
    sensorData[0].isValid = true;
    sensorData[0].value = temperature;
    sensorData[0].unit = "*C";
    sensorData[0].name = "Temperature";

    sensorData[1].isValid = true;
    sensorData[1].value = pressure;
    sensorData[1].unit = "mbar";
    sensorData[1].name = "Pressure";

    sensorData[2].isValid = true;
    sensorData[2].value = humidity;
    sensorData[2].unit = "%";
    sensorData[2].name = "Humidity";

    return sensorData;
}

std::array<SensorData, 3> getCjmcu()
{
    std::array<SensorData, 3> sensorData;
    sensorData.fill(SensorData());

    if (ccs.available())
    {
        while (!ccs.available())
        {
            delay(0.1);
        }
        auto res = ccs.readData();
        for (int i = 0; i < 5; i++)
        {
            if (res == 0)
            {
                break;
            }
            res = ccs.readData();
            delay(0.3);
        }
        if (!res)
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
            Serial.println(sensorData[0].value);
            Serial.print("TVOC: ");
            Serial.println(sensorData[1].value);
        }
        else
        {
            Serial.println("cjmcu ERROR");
            return sensorData;
        }
    }
    return sensorData;
}

std::array<SensorData, 3> getQMP6988()
{
    std::array<SensorData, 3> sensorData;
    //TODO
    return sensorData;
}

std::array<SensorData, 3> getBMP280()
{
    std::array<SensorData, 3> sensorData;
    sensorData.fill(SensorData());
    if (!bmp.begin(0x70))
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
    return sensorData;
}

std::array<SensorData, 3> getSht30()
{
    std::array<SensorData, 3> sensorData;
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
    sensorData[0].isValid = true;
    sensorData[0].value = humidity;
    sensorData[0].unit = "%";
    sensorData[0].name = "Humidity";

    return sensorData;
}

void initI2CSensor(uint8_t address)
{
    Serial.print("Init Sensor: ");
    Serial.println(address);
    delay(1.0);
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
            m_SensorTypes.emplace_back(SensorType::bmp280);
        }
        else
        {
            m_SensorTypes.emplace_back(SensorType::bme280);
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

        Serial.println("cjmcu: wait till available");
        //calibrate temperature sensor
        while (!ccs.available())
        {
            delay(0.1);
        }
        Serial.println("cjmcu: Calc temperature");
        float cTemp = ccs.calculateTemperature();
        ccs.setTempOffset(cTemp - 25.0);
        getCjmcu();
        Serial.println("cjmcu: Add to sensors");
        m_SensorTypes.emplace_back(SensorType::cjmcu);
    }
    else if (address == 0x44)
    {
        Serial.println("Sensor SHT30.");
        m_SensorTypes.emplace_back(SensorType::sht30);
    }
    else if (address == 0x58)
    {
        Serial.println("Sensor SGP30 not yet implemented.");
        /*
        Serial.println("Sensor SGP30.");
        Serial.print("Init SGP30 CO2 / TVOC");
        m_SensorTypes.emplace_back(SensorType::sgp30);
        if (!sgp.begin(&MyWire))
        {
            Serial.println("Init SGP30 Failed");
        };
        Serial.print(sgp.serialnumber[0], HEX);
        Serial.print(sgp.serialnumber[1], HEX);
        Serial.println(sgp.serialnumber[2], HEX);
        */
    }
}
