/*
 * RemoteDebug with Telnet and AsyncWebSocket.
 * Install ESPAsyncWebServer for ESP32.
 * REMOTEDEBUG_ENABLE_WEBSOCKET must be enabled in RemoteDebugCfg.h or set in platformio.ini build flags.
 * build flags:
 * -D REMOTEDEBUG_ENABLE_WEBSOCKET=1
 * 
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <RemoteDebug.h>

constexpr char WIFI_SSID[] = "YOUR_WIFI_SSID";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
constexpr char HOST_NAME[] = "remote-debug-dual";

AsyncWebServer webServer(80);
RemoteDebug Debug;

void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOST_NAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
        Serial.print('.');
    }

    Serial.println();
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void setup() {
    Serial.begin(115200);
    connectWiFi();

    // Start the Telnet transport.
    if (!Debug.begin(HOST_NAME, RemoteDebug::DEBUG)) {
        Serial.println("RemoteDebug Telnet failed to start");
    }

    // Configure the WebSocket transport independently.
    Debug.setWebDebugLevel(RemoteDebug::DEBUG);
    Debug.showWebDebugLevel(true);
    Debug.showWebProfiler(false);

    // Register the built-in page and /debug endpoint.
    if (!Debug.beginWebSocket(&webServer, HOST_NAME, "/debug")) {
        Serial.println("RemoteDebug WebSocket page failed to start");
    }

    webServer.begin();

    Serial.printf(
        "Telnet: telnet %s %u\n",
        WiFi.localIP().toString().c_str(),
        TELNET_PORT);

    Serial.printf(
        "Browser: http://%s/debug\n",
        WiFi.localIP().toString().c_str());

    debugI("Dual-stack RemoteDebug started");
}

void loop() {
  Debug.handle();
  static uint32_t lastLog = 0;
  if ((uint32_t)(millis() - lastLog) >= 5000) {
    lastLog = millis();
    debugD("Uptime: %lu ms", (unsigned long)millis());
    }
}
