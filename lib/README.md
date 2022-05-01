

```
cd ./lib/
git clone https://github.com/adafruit/Adafruit_CCS811.git
git clone https://github.com/WifWaf/MH-Z19.git
git clone https://github.com/tobiasschuerg/MH-Z-CO2-Sensors
mkdir MH-Z19/src
mv ./MH-Z19/MH-Z19.* src 
git clone https://github.com/adafruit/Adafruit_NeoPixel.git
git clone https://github.com/adafruit/Adafruit_Sensor.git
git clone https://github.com/adafruit/DHT-sensor-library.git
git clone https://github.com/adafruit/Adafruit_BusIO.git
git clone https://github.com/adafruit/Adafruit_BME280_Library.git
git clone https://github.com/adafruit/Adafruit_BMP280_Library.git
git clone https://github.com/bblanchon/ArduinoJson.git
git clone https://github.com/arduino-libraries/NTPClient.git
git clone  https://github.com/plerup/espsoftwareserial
mkdir NTPClient/src
mv ./NTPClient/NTPClient.* src 
git clone https://github.com/sparkfun/SparkFun_SCD30_Arduino_Library.git SCD30


```
## Little FS
https://github.com/earlephilhower/mklittlefs/releases
Terminal > Run Task > Platform IO: Upload Filesystem