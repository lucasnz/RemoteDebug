/*
 * RemoteDebug Basic example — ESP32
 *
 * Connect to the ESP32 over Telnet on port 23, then change the log level
 * from the Telnet client. For WebSocket output, see ../DualTransport.
 */
#include <WiFi.h>
#include <ESPmDNS.h>
#include <RemoteDebug.h>

static constexpr char HOST_NAME[] = "remotedebug";
static constexpr char WIFI_SSID[] = "your-ssid";
static constexpr char WIFI_PASSWORD[] = "your-password";

RemoteDebug Debug;

uint32_t lastTick = 0;
uint32_t uptimeSeconds = 0;

static void demonstrateFunction();
static void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print('.');
  }

  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin(HOST_NAME)) {
    MDNS.addService("telnet", "tcp", 23);
    Serial.printf("mDNS: %s.local\n", HOST_NAME);
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  connectWiFi();

  Debug.begin(HOST_NAME, RemoteDebug::VERBOSE);
  Debug.setResetCmdEnabled(true);
  Debug.showColors(true);

  Serial.println("RemoteDebug Basic is ready.");
  Serial.println("Connect using a Telnet client to the ESP32 IP, port 23.");
}

void loop() {
  Debug.handle();

  if ((uint32_t)(millis() - lastTick) >= 1000) {
    lastTick = millis();
    ++uptimeSeconds;

    debugV("Uptime: %lu seconds", (unsigned long)uptimeSeconds);

    if ((uptimeSeconds % 5) == 0) {
      debugD("Example DEBUG message");
      debugI("Example INFO message");
      debugW("Example WARNING message");
      debugE("Example ERROR message");
      demonstrateFunction();
    }
  }

  delay(1);
}

static void demonstrateFunction() {
  debugV("A VERBOSE message emitted by demonstrateFunction()");
}
