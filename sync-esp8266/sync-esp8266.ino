#include "ESP8266WiFi.h"
#include "WiFiClientSecure.h"

#include <PubSubClient.h>
#include <ArduinoJson.h>

/*
 * Itead Sonoff DEV blue LED is on GPIO13
 * Wio Link blue LED is on GPIO2
 */
#define STATUS_LED 13

/*
 * Itead Sonoff DEV Grove power enable is on GPIO16
 * Wio Link Grove power enable is on GPIO15
 */
#define GROVE_POWER 16

/*
 * Assume that Grove relay output is on GPIO5
 */
#define RELAY_SIG 5

/*
 * WiFi Setings
 *
 * Provide according to your network configuration.
 */
const char* wifiSsid = "YOUR_WIFI_SSID";
const char* wifiPassword = "YOUR_WIFI_PASSWORD";

/*
 * Sync Settings
 *
 * Enter a Sync for IoT Key & Secret.
 * Provide the MQTT topic with the Sync List unique name populated by
 * the Studio workflow.
 */
const char* syncDeviceKey = "YOUR_DEVICE_KEY";
const char* syncDeviceSecret = "YOUR_DEVICE_SECRET";
const char* syncTopic = "sync/lists/OpenSesame";

/*
 * Sync server and MQTT setup; you probably don't have to change these.
 */
const char* mqttServer = "mqtt-sync.us1.twilio.com";
const char* mqttClientId = "GarageDoorOpener";
const uint16_t mqttPort = 8883;
const uint16_t maxPacketSize = 512;

void topicCallback(char*, byte*, unsigned int);
WiFiClientSecure espClient;
PubSubClient client(mqttServer, mqttPort, topicCallback, espClient);

/*
 * Our Sync for IoT message handling callback. This is passed as a
 * callback function when we subscribe to the list, and any new messages will
 * appear here.
 */
void topicCallback(char* topic, byte* payload, unsigned int length)
{
        std::unique_ptr<char []> msg(new char[length+1]());
        memcpy(msg.get(), payload, length);

        Serial.print("Topic: [");
        Serial.print(topic);
        Serial.print("] mesage: ");
        Serial.println(msg.get());

        StaticJsonBuffer<maxPacketSize> jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(msg.get());
        String caller = root["caller"];

        if (caller.length() > 0) {
            Serial.print("Identified caller: ");
            Serial.println(caller.c_str());
            digitalWrite(RELAY_SIG, HIGH);
            delay(1000);
            digitalWrite(RELAY_SIG, LOW);
        }
}

/*
 * This function connects to Sync via MQTT. We connect using the key, password, and
 * device name defined as constants above, and immediately check the server's
 * certificate fingerprint (if desired).
 *
 * If everything works, we subscribe to the document topic and return.
 */
void connectMqtt()
{
        while (!client.connected()) {
                Serial.println("Attempting to connect to Twilio Sync...");
                if (client.connect(mqttClientId, syncDeviceKey, syncDeviceSecret)) {
                        Serial.print("Connected! Subscribing to ");
                        Serial.println(syncTopic);
                        client.subscribe(syncTopic);
                        digitalWrite(STATUS_LED, LOW);
                } else {
                        digitalWrite(STATUS_LED, HIGH);
                        Serial.print("failed, rc=");
                        Serial.print(client.state());
                        delay(10000);
                }
        }
}

/*
 * During setup, we configure our LED for output, and start blinking it while connecting to WiFi.
 */
void setup()
{
        pinMode(STATUS_LED, OUTPUT);
        digitalWrite(STATUS_LED, HIGH);

        pinMode(GROVE_POWER, OUTPUT);
        digitalWrite(GROVE_POWER, LOW);

        pinMode(RELAY_SIG, OUTPUT);
        digitalWrite(RELAY_SIG, LOW);

        Serial.begin(115200);
        WiFi.begin(wifiSsid, wifiPassword);

        while (WiFi.status() != WL_CONNECTED) {
                digitalWrite(STATUS_LED, LOW);
                delay(1000);
                digitalWrite(STATUS_LED, HIGH);
                Serial.print(".");
        }

        randomSeed(micros());

        Serial.print("\nWiFi connected! IP address: ");
        Serial.println(WiFi.localIP());
}

/*
 * Our loop constantly checks we are still connected. On disconnects we try again.
 */
void loop()
{
        if (!client.connected()) {
                connectMqtt();
        }
        client.loop();
}

