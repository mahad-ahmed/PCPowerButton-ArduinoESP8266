#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

//  LOW means ON on a NodeMCU, below defs are for better readability
#define LED_ON LOW
#define LED_OFF HIGH

//  Pins respective to a NodeMCU
#define POWER_PIN 0     // D3
#define RESTART_PIN 4   // D2
#define STATUS_PIN 5    // D1

#define CMD_PORT 6943
#define STATUS_PORT 6947

//  These are the bytes that will be received as commands from the app
#define START 100
#define RESTART 101
#define FORCED_SHUTDOWN 102
#define CHECK_STATUS 103

//  Your network name and password goes here
#define SSID "[REDACTED]"
#define PASSWORD "[REDACTED]"

WiFiUDP udp;
IPAddress broadcastIP;
int prevStatus;

void setup() {
    pinMode(STATUS_PIN, INPUT);
    prevStatus = digitalRead(STATUS_PIN);

    // Set power pins to default HIGH state (HIGH means off!)
    pinMode(POWER_PIN, OUTPUT);
    pinMode(RESTART_PIN, OUTPUT);
    digitalWrite(POWER_PIN, HIGH);
    digitalWrite(RESTART_PIN, HIGH);

    // Turn LED ON indicating connection attempt
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_ON);

    // Attempt connection, blocks until successfully connected
    WiFi.begin(SSID, PASSWORD);
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    // Get broadcast address for local network
    broadcastIP = WiFi.localIP();
    broadcastIP[3] = 255;

    // Start listening for UDP packets on PORT
    udp.begin(CMD_PORT);

    // Turn LED OFF indicating successful connection and socket creation
    digitalWrite(LED_BUILTIN, LED_OFF);
}

void sendPulse(uint8_t pin, unsigned long pulseDelay) {
    digitalWrite(pin, LOW);
    digitalWrite(LED_BUILTIN, LED_ON);
    delay(pulseDelay);
    digitalWrite(pin, HIGH);
    digitalWrite(LED_BUILTIN, LED_OFF);
}

void loop() {
    int size = udp.parsePacket();
    if(size > 0) {
        int cmd = udp.read();
        if(cmd == CHECK_STATUS) {
            udp.beginPacket(udp.remoteIP(), STATUS_PORT);
            udp.write(digitalRead(STATUS_PIN));  // Can reuse "status" here
            udp.endPacket();
        }
        else if(cmd == START) {
            sendPulse(POWER_PIN, 350);
        }
        else if(cmd == RESTART) {
            sendPulse(RESTART_PIN, 350);
        }
        else if(cmd == FORCED_SHUTDOWN) {
            sendPulse(POWER_PIN, 4020);
        }
    }

    int status = digitalRead(STATUS_PIN);
    if(status != prevStatus) {
        // Broadcast new status
        udp.beginPacket(broadcastIP, STATUS_PORT);
        udp.write(status);
        udp.endPacket();
        
        prevStatus = status;
    }
}