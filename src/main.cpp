#include <Arduino.h>
#include <ConfigPortal32.h>
#include <PubSubClient.h>
#include <TFT_eSPI.h>
#include <DHTesp.h>
#include <Wire.h>
#include <BH1750.h>

char*               ssid_pfix = (char*)"mqtt_sensor_";
String              user_config_html = ""
    "<p><input type='text' name='broker' placeholder='MQTT Server'>";

char                mqttServer[100];
const int           mqttPort = 1883;
unsigned long       pubInterval = 5000;
unsigned long       lastPublished = - pubInterval;

#define             SDA_PIN 44  // Define your custom SDA pin
#define             SCL_PIN 43  // Define your custom SCL pin

BH1750              lightMeter;
float               lux = 0;
char                buf_lux[50];

#define             DHTPIN 18
DHTesp              dht;
int                 dht_interval = 2000;
unsigned long       lastDHTReadMillis = 0;
float               humidity = 0;
float               temperature = 0;
char                buf_temp[50];
char                buf_humi[50];

TFT_eSPI            tft = TFT_eSPI();

WiFiClient wifiClient;
PubSubClient client(wifiClient);
void pubStatus();

void readDHT22() {
    unsigned long currentMillis = millis();

    if(millis() > lastDHTReadMillis + dht_interval) {
        lastDHTReadMillis = currentMillis;

        humidity = dht.getHumidity();
        temperature = dht.getTemperature();
    }
}

void setup() {
    Serial.begin(115200);

    loadConfig();
    // *** If no "config" is found or "config" is not "done", run configDevice ***
    if(!cfg.containsKey("config") || strcmp((const char*)cfg["config"], "done")) {
        configDevice();
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin((const char*)cfg["ssid"], (const char*)cfg["w_pw"]);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    // main setup
    dht.setup(DHTPIN, DHTesp::DHT22);   // Connect DHT Sensr to GPIO 18
    Wire.begin(SDA_PIN, SCL_PIN);  // Initialize I²C with custom pins
    lightMeter.begin();
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Thermostat", 80, 20, 4);
    tft.drawString("Temp : ", 30, 60, 2);
    tft.drawString("Humi : ", 30, 90, 2);
    tft.drawString("Lux  : ", 30, 120, 2);
    Serial.printf("\nIP address : "); Serial.println(WiFi.localIP());

    if (cfg.containsKey("broker")) {
            sprintf(mqttServer, (const char*)cfg["broker"]);
    }
    client.setServer(mqttServer, mqttPort);

    while (!client.connected()) {
        Serial.println("Connecting to MQTT...");
        if (client.connect("MQTTSensor")) {
            Serial.println("connected");  
        } else {
            Serial.print("failed with state "); Serial.println(client.state());
            delay(2000);
        }
    }
}

void loop() {
    client.loop();

    unsigned long currentMillis = millis();
    if(currentMillis - lastPublished >= pubInterval) {
        lastPublished = currentMillis;
        readDHT22();
        lux = lightMeter.readLightLevel();
        sprintf(buf_lux, "%.2f", lux);
        sprintf(buf_temp, "%.1f", temperature);
        sprintf(buf_humi, "%.1f", humidity);
        tft.fillRect(90, 60, 90, 90, TFT_BLACK);
        tft.drawString(buf_temp, 90, 60, 2);
        tft.drawString(buf_humi, 90, 90, 2);
        tft.drawString(buf_lux, 90, 120, 2);
        pubStatus();
    }
}

void pubStatus() {
    client.publish("id/yourname/sensor/evt/temperature", buf_temp);
    client.publish("id/yourname/sensor/evt/humidity", buf_humi);
    client.publish("id/yourname/sensor/evt/lux", buf_lux);
}
