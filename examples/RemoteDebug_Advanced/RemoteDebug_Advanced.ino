/*
 * RemoteDebug Advanced example — ESP32
 *
 * Demonstrates project commands, a callback, explicit level checks, filtering,
 * and benchmark output. Connect with Telnet to port 23 and enter "help".
 */
#include <WiFi.h>
#include <ESPmDNS.h>
#include <RemoteDebug.h>

static constexpr char HOST_NAME[] = "remotedebug-advanced";
static constexpr char WIFI_SSID[] = "your-ssid";
static constexpr char WIFI_PASSWORD[] = "your-password";

RemoteDebug Debug;

uint32_t lastTick = 0;
uint32_t tickCount = 0;

static void connectWiFi();
static void processProjectCommand();
static void runPrintBenchmark();
static void runPrintlnBenchmark();

void setup() {
  Serial.begin(115200);
  delay(100);

  connectWiFi();

  Debug.begin(HOST_NAME, RemoteDebug::VERBOSE);
  Debug.setResetCmdEnabled(true);
  Debug.setPassword("");  // Set a non-empty password if needed; Telnet is not encrypted.
  Debug.showColors(true);
  Debug.showTime(true);
  Debug.showProfiler(true, 0);

  Debug.setHelpProjectsCmds(
      "bench1 - Benchmark formatted debug output\n"
      "bench2 - Benchmark print/println output\n"
      "status - Show Wi-Fi and uptime status");
  Debug.setCallBackProjectCmds(processProjectCommand);

  Serial.println("RemoteDebug Advanced is ready.");
  Serial.printf("Connect to %s on port 23.\n", WiFi.localIP().toString().c_str());
}

void loop() {
  Debug.handle();

  if ((uint32_t)(millis() - lastTick) >= 1000) {
    lastTick = millis();
    ++tickCount;

    debugV("Uptime: %lu seconds", (unsigned long)tickCount);

    if ((tickCount % 5) == 0) {
      debugV("VERBOSE level example");
      debugD("DEBUG level example");
      debugI("INFO level example");
      debugW("WARNING level example");
      debugE("ERROR level example");

      // Only perform the extra work if the current client level requests it.
      if (Debug.isActive(RemoteDebug::DEBUG)) {
        Debug.println("Debug level is active; calling benchmark helper.");
        debugD("This line includes the helper's function name.");
      }
    }
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

static void processProjectCommand() {
  const String command = Debug.getLastCommand();

  if (command == "bench1") {
    runPrintBenchmark();
  } else if (command == "bench2") {
    runPrintlnBenchmark();
  } else if (command == "status") {
    debugA("Wi-Fi: %s; IP: %s; uptime: %lu seconds",
           WiFi.isConnected() ? "connected" : "disconnected",
           WiFi.localIP().toString().c_str(),
           (unsigned long)tickCount);
  } else {
    debugW("Unknown project command: %s", command.c_str());
  }

  Debug.clearLastCommand();
}

static void runPrintBenchmark() {
  constexpr uint16_t iterations = 50;
  const uint32_t start = millis();

  for (uint16_t i = 0; i < iterations; ++i) {
    debugA("%u - formatted debug output", i);
  }

  debugA("bench1: %u messages in %lu ms",
         iterations, (unsigned long)(millis() - start));
}

static void runPrintlnBenchmark() {
  constexpr uint16_t iterations = 50;
  const uint32_t start = millis();

  for (uint16_t i = 0; i < iterations; ++i) {
    if (Debug.isActive(RemoteDebug::ANY)) {
      Debug.print(i);
      Debug.println(" - print/println output");
    }
  }

  debugA("bench2: %u messages in %lu ms",
         iterations, (unsigned long)(millis() - start));
}
