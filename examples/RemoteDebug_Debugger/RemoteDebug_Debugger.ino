/*
 * RemoteDebug Diagnostics example — ESP32
 *
 * This is a small interactive diagnostic example that uses RemoteDebug's
 * project-command callback. It deliberately does not depend on the separate
 * legacy RemoteDebugger add-on.
 *
 * Connect by Telnet to port 23 and enter "help" for available commands.
 */
#include <WiFi.h>
#include <ESPmDNS.h>
#include <RemoteDebug.h>

static constexpr char HOST_NAME[] = "remotedebug-diagnostics";
static constexpr char WIFI_SSID[] = "your-ssid";
static constexpr char WIFI_PASSWORD[] = "your-password";

RemoteDebug Debug;

bool diagnosticFlag = false;
int diagnosticValue = 42;
uint32_t lastTick = 0;
uint32_t uptimeSeconds = 0;

static void connectWiFi();
static void processDiagnosticCommand();

void setup() {
  Serial.begin(115200);
  delay(100);

  connectWiFi();

  Debug.begin(HOST_NAME, RemoteDebug::DEBUG);
  Debug.setResetCmdEnabled(true);
  Debug.showColors(true);
  Debug.showTime(true);

  Debug.setHelpProjectsCmds(
      "status - Print runtime and Wi-Fi status\n"
      "vars - Print diagnostic variables\n"
      "toggle - Toggle diagnosticFlag\n"
      "inc - Increment diagnosticValue\n"
      "bench - Run a short logging benchmark");
  Debug.setCallBackProjectCmds(processDiagnosticCommand);

  Serial.println("RemoteDebug Diagnostics is ready.");
  Serial.printf("Connect to %s on port 23.\n", WiFi.localIP().toString().c_str());
}

void loop() {
  Debug.handle();

  if ((uint32_t)(millis() - lastTick) >= 1000) {
    lastTick = millis();
    ++uptimeSeconds;
    debugV("Uptime: %lu seconds", (unsigned long)uptimeSeconds);
  }

  delay(1);
}

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

static void processDiagnosticCommand() {
  const String command = Debug.getLastCommand();

  if (command == "status") {
    debugA("Wi-Fi=%s, IP=%s, uptime=%lu s",
           WiFi.isConnected() ? "connected" : "disconnected",
           WiFi.localIP().toString().c_str(),
           (unsigned long)uptimeSeconds);
  } else if (command == "vars") {
    debugA("diagnosticFlag=%s, diagnosticValue=%d",
           diagnosticFlag ? "true" : "false", diagnosticValue);
  } else if (command == "toggle") {
    diagnosticFlag = !diagnosticFlag;
    debugI("diagnosticFlag is now %s", diagnosticFlag ? "true" : "false");
  } else if (command == "inc") {
    ++diagnosticValue;
    debugI("diagnosticValue=%d", diagnosticValue);
  } else if (command == "bench") {
    const uint32_t start = millis();
    for (uint16_t i = 0; i < 25; ++i) {
      debugD("Diagnostic benchmark line %u", i);
    }
    debugA("Benchmark completed in %lu ms", (unsigned long)(millis() - start));
  } else {
    debugW("Unknown diagnostic command: %s", command.c_str());
  }

  Debug.clearLastCommand();
}
