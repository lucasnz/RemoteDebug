/*
 * RemoteDebug with Telnet and AsyncWebSocket using one RemoteDebug instance.
 * Install ESPAsyncWebServer and AsyncTCP for ESP32.
 * REMOTEDEBUG_ENABLE_WEBSOCKET is enabled in RemoteDebugCfg.h by default.
 */
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <RemoteDebug.h>

const char* WIFI_SSID = "your-ssid";
const char* WIFI_PASSWORD = "your-password";

AsyncWebServer server(80);
AsyncWebSocket debugSocket("/debug");
RemoteDebug Debug;

void onDebugSocketEvent(AsyncWebSocket* socket, AsyncWebSocketClient* client,
                        AwsEventType type, void* arg, uint8_t* data, size_t len) {
  (void)socket; (void)arg;
  if (type == WS_EVT_CONNECT) {
    Debug.sendWebMessage(client, "RemoteDebug WebSocket connected");
  } else if (type == WS_EVT_DATA) {
    // Logging is shared by both transports. Add an application-specific,
    // authenticated command protocol here if commands are needed.
    String message;
    message.reserve(len);
    for (size_t i = 0; i < len; ++i) message += (char)data[i];
    Debug.sendWebMessage(client, String("Command received: ") + message);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(250);

  Debug.begin("esp32-debug", RemoteDebug::DEBUG);
  Debug.attachWebSocket(&debugSocket);
  debugSocket.onEvent(onDebugSocketEvent);
  server.addHandler(&debugSocket);
  server.begin();

  debugI("RemoteDebug dual-transport ready");
  debugW("Warnings appear in Telnet and WebSocket clients");
}

void loop() {
  Debug.handle();
  static uint32_t lastLog = 0;
  if ((uint32_t)(millis() - lastLog) >= 5000) {
    lastLog = millis();
    debugD("Uptime: %lu ms", (unsigned long)millis());
  }
}
