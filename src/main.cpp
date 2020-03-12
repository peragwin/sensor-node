#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#define MQTT_MAX_PACKET_SIZE 1024
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h>

#define SD_SS 13
#define SD_MOSI 15
#define SD_SCK 14
#define SD_MISO 2

#define I2C_SDA 19
#define I2C_SCL 22

#define WIFI_SSID_PATH "/config/wifi_ssid"
#define WIFI_PASS_PATH "/config/wifi_pass"
#define MQTT_HOST_PATH "/config/mqtt_host"
#define DEVICE_NAME_PATH "/config/name"

SPIClass SDSPI(HSPI);
TwoWire i2c(0);

Adafruit_BME280 bme;

WiFiClient net;
PubSubClient mqtt(net);
String deviceName;
bool inited = false;

String readFile(const char *name)
{
  File file = SD.open(name);
  if (!file)
  {
    Serial.printf("Could not open: %s\n", name);
    return "";
  }
  String ret = file.readString();
  file.close();
  return ret;
}

bool connectWifi()
{
  String ssid = readFile(WIFI_SSID_PATH);
  if (!ssid.length())
  {
    return false;
  }
  String pass = readFile(WIFI_PASS_PATH);
  if (!pass.length())
  {
    return false;
  }

  WiFiClass wifi;
  if (!wifi.begin(ssid.c_str(), pass.c_str()))
  {
    return false;
  }
  while (!wifi.isConnected())
  {
    delay(500);
    Serial.print(".");
  }
  return true;
}

bool connectMQTT()
{
  String host = readFile(MQTT_HOST_PATH);
  if (!host.length())
  {
    return false;
  }
  uint8_t ip[4];
  int c = sscanf(host.c_str(), "%u.%u.%u.%u", ip, ip + 1, ip + 2, ip + 3);
  Serial.println("scanned");
  Serial.println(c);
  if (c == 4)
  {
    IPAddress addr(ip[0], ip[1], ip[2], ip[3]);
    mqtt.setServer(addr, 1883);
  }
  else
  {
    mqtt.setServer(host.c_str(), 1883);
  }
  net.setTimeout(10);
  return true;
}

void setup()
{
  Serial.begin(115200);
  SDSPI.begin(SD_SCK, SD_MISO, SD_MOSI, -1);
  if (!SD.begin(SD_SS, SDSPI))
  {
    Serial.println("Failed to mount SD card");
    return;
  }
  Serial.println("Opened SD card");

  if (!i2c.begin(I2C_SDA, I2C_SCL))
  {
    Serial.println("Failed to start I2C");
    return;
  }
  if (!bme.begin(0x76, &i2c))
  {
    Serial.println("Failed to start BME280");
    // for (int i = 0; i < 127; i++)
    // {
    //   i2c.beginTransmission(i);
    //   uint8_t err = i2c.endTransmission();
    //   if (err == 0)
    //   {
    //     Serial.printf("found device at %d\n", i);
    //   }
    //   else if (err == 4)
    //   {
    //     Serial.printf("error device at %d\n", i);
    //   }
    // }
    return;
  }
  Serial.println("Initialized BME280 sensor");

  deviceName = readFile(DEVICE_NAME_PATH);
  if (!deviceName.length())
  {
    return;
  }

  if (!connectWifi())
  {
    Serial.println("Failed to connect to WIFI");
    return;
  }
  Serial.println("Connected to WIFI");

  if (!connectMQTT())
  {
    Serial.println("Failed to get MQTT host");
    return;
  }

  inited = true;
}

StaticJsonDocument<200> notification(const char *path)
{
  StaticJsonDocument<200> doc;
  doc["device"] = deviceName;
  doc["path"] = path;
  return doc;
}

void reconnect()
{
  String pub = "device/" + deviceName + "/connection";
  StaticJsonDocument<200> notif = notification("/connection");
  String out;

  notif["connected"] = 0;
  serializeJson(notif, out);

  while (!mqtt.connected())
  {
    String id = deviceName + "-" + String(random(0xffff), HEX);
    if (mqtt.connect(id.c_str(), pub.c_str(), 1, true, out.c_str()))
    {
      Serial.println("MQTT connected!");

      notif["connected"] = 1;
      String out2;
      serializeJson(notif, out2);
      mqtt.publish(pub.c_str(), out2.c_str());

      Serial.println("published maybe?");
      Serial.println(mqtt.connected());
      return;
    }
    else
    {
      Serial.println("MQTT failed to connect");
      Serial.println(mqtt.state());
      delay(5000);
    }
  }
}

void publishSensorValues()
{
  float temp = bme.readTemperature();
  float bar = bme.readPressure();
  float hum = bme.readHumidity();
  StaticJsonDocument<200> notif = notification("/sensors/bme280");
  notif["temp"] = temp;
  notif["pressure"] = bar;
  notif["humidity"] = hum;
  String out;
  serializeJson(notif, out);
  String pub = "device/" + deviceName + "/sensors/bme280";
  if (!mqtt.publish(pub.c_str(), out.c_str()))
  {
    Serial.println("failed to publish bme data");
    Serial.println(out);
  }
  Serial.printf("Temp = %f, Pres = %f, Hum = %f\n", temp, bar, hum);
}

long lastUpdate = -12345678;

void loop()
{
  if (!inited)
    return;

  // Serial.println("loop1");
  if (!mqtt.loop())
    reconnect();

  long now = millis();
  if (now - lastUpdate > 10000)
  {
    publishSensorValues();
    lastUpdate = now;
  }
  delay(100);
  // Serial.println("loop2");
  // Serial.print("net conn: ");
  // Serial.println(net.connected());
}