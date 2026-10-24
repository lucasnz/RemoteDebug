# RemoteDebug

**Remote debugging for ESP32 over Telnet, WebSockets, or both, using one logging API.**

This project is a fork of [JoaoLopesF/RemoteDebug](https://github.com/JoaoLopesF/RemoteDebug) and incorporates changes from [ktos/RemoteDebug](https://github.com/ktos/RemoteDebug).

## Features

- ESP32 support using the Arduino framework.
- Telnet logging on a configurable runtime port.
- Optional browser-based WebSocket console served directly by the library.
- Telnet-only, WebSocket-only, and dual-transport operation.
- Independent Telnet and WebSocket debug levels and display settings.
- Common command set across Telnet and WebSocket clients.
- Standard `debugV()`, `debugD()`, `debugI()`, `debugW()`, `debugE()`, and `debugA()` logging macros.
- Optional timestamps, colours, profiler timing, filters, and Serial mirroring.
- Project-specific commands, command help, and callbacks.
- Optional password prompt with Telnet echo suppression.
- Bounded Telnet command input and WebSocket output buffers.
- Compile-time removal through `DEBUG_DISABLED`.

## Platform and dependencies

RemoteDebug targets **ESP32 only**. Compilation intentionally fails on unsupported targets.

### Telnet-only requirements

- ESP32 Arduino framework
- Wi-Fi connectivity

### Additional WebSocket requirements

- A compatible `ESPAsyncWebServer` installation
- The asynchronous TCP dependency required by that server version

WebSocket dependencies are only required when WebSocket support is enabled.

## Installation

### PlatformIO

Add the library to `platformio.ini`:

```ini
lib_deps =
    https://github.com/lucasnz/RemoteDebug.git
```

For WebSocket support, also add a compatible asynchronous web server and enable the transport:

```ini
lib_deps =
    https://github.com/lucasnz/RemoteDebug.git
    ESP32Async/ESPAsyncWebServer

build_flags =
    -D REMOTEDEBUG_ENABLE_WEBSOCKET=1
```

Avoid installing conflicting versions of `ESPAsyncWebServer` or its asynchronous TCP dependency.

### Arduino IDE

1. Download or clone this repository.
2. Install it in the Arduino libraries directory.
3. If WebSocket support is required, install a compatible `ESPAsyncWebServer` and its dependencies.
4. Include `RemoteDebug.h` in the sketch.

## Logging

```cpp
debugV("Verbose message");
debugD("Debug message");
debugI("Informational message");
debugW("Warning message");
debugE("Error message");
debugA("Always visible at any configured level");
```

Available levels, from most to least verbose, are:

- `VERBOSE`
- `DEBUG`
- `INFO`
- `WARNING`
- `ERROR`
- `ANY`

The logging macros include the current function name and ESP32 core ID unless automatic function prefixes are disabled.

The library is designed for a single application task. Do not call `handle()`, logging macros, or direct `Print` methods concurrently from multiple tasks without external serialisation.

## Telnet-only usage

WebSocket support can remain disabled for a Telnet-only build.

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <RemoteDebug.h>

constexpr char WIFI_SSID[] = "YOUR_WIFI_SSID";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
constexpr char HOST_NAME[] = "remote-debug-telnet";

RemoteDebug Debug;

void connectWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOST_NAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(250);
    }
}

void setup()
{
    Serial.begin(115200);
    connectWiFi();

    if (!Debug.begin(HOST_NAME, 23, RemoteDebug::DEBUG))
    {
        Serial.println("RemoteDebug failed to start");
        return;
    }

    Debug.showDebugLevel(true);
    Debug.showTime(false);
    Debug.showProfiler(false);

    debugI("Telnet RemoteDebug started");
}

void loop()
{
    Debug.handle();
    delay(5);
}
```

Connect using:

```text
telnet <ESP32-IP> 23
```

### Selecting the Telnet port and initial level

The default overload uses `TELNET_PORT` and starts at `DEBUG` level:

```cpp
Debug.begin("my-device");
```

Select an initial level:

```cpp
Debug.begin("my-device", RemoteDebug::INFO);
```

Select both a runtime port and initial level:

```cpp
Debug.begin("my-device", 2323, RemoteDebug::INFO);
```

`begin()` returns `false` if the port is zero, the debug level is invalid, or the Telnet server cannot be created.

## WebSocket-only usage

WebSocket support must be enabled at compile time:

```ini
build_flags =
    -D REMOTEDEBUG_ENABLE_WEBSOCKET=1
```

The application owns the `AsyncWebServer`. RemoteDebug creates and owns the `/debug/ws` WebSocket endpoint and registers its built-in browser page with the supplied server.

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <RemoteDebug.h>

constexpr char WIFI_SSID[] = "YOUR_WIFI_SSID";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
constexpr char HOST_NAME[] = "remote-debug-web";

AsyncWebServer webServer(80);
RemoteDebug Debug;

void connectWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOST_NAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(250);
    }
}

void setup()
{
    Serial.begin(115200);
    connectWiFi();

    Debug.setWebDebugLevel(RemoteDebug::DEBUG);
    Debug.showWebDebugLevel(true);
    Debug.showWebProfiler(false);

    // The hostname is supplied here because Debug.begin() is not used.
    if (!Debug.beginWebSocket(&webServer, HOST_NAME))
    {
        Serial.println("RemoteDebug WebSocket failed to start");
        return;
    }

    webServer.begin();

    Serial.printf(
        "Open http://%s/debug\n",
        WiFi.localIP().toString().c_str());

    debugI("WebSocket RemoteDebug started");
}

void loop()
{
    Debug.handle();
    delay(5);
}
```

The default browser page is available at:

```text
http://<ESP32-IP>/debug
```

The WebSocket endpoint is:

```text
ws://<ESP32-IP>/debug/ws
```

To serve the browser page at a different path:

```cpp
Debug.beginWebSocket(&webServer, HOST_NAME, "/remote-debug");
```

The WebSocket endpoint remains `/debug/ws`.

## Dual-transport usage

Telnet and WebSocket clients can operate at the same time. The transports retain independent debug levels and WebSocket-specific display settings.

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <RemoteDebug.h>

constexpr char WIFI_SSID[] = "YOUR_WIFI_SSID";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
constexpr char HOST_NAME[] = "remote-debug-dual";

AsyncWebServer webServer(80);
RemoteDebug Debug;

void connectWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOST_NAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(250);
    }
}

void setup()
{
    Serial.begin(115200);
    connectWiFi();

    if (!Debug.begin(HOST_NAME, 23, RemoteDebug::DEBUG))
    {
        Serial.println("RemoteDebug Telnet failed to start");
    }

    Debug.setWebDebugLevel(RemoteDebug::DEBUG);
    Debug.showWebDebugLevel(true);
    Debug.showWebProfiler(false);

    if (!Debug.beginWebSocket(&webServer, HOST_NAME))
    {
        Serial.println("RemoteDebug WebSocket failed to start");
    }

    webServer.begin();

    debugI("Dual-transport RemoteDebug started");
}

void loop()
{
    Debug.handle();
    delay(5);
}
```

## Commands

Telnet and WebSocket clients support the same core command set:

- `?`, `h`, or `help`: show command help
- `q`: close the current client session
- `m`: show free heap
- `v`: set the transport debug level to verbose
- `d`: set the transport debug level to debug
- `i`: set the transport debug level to info
- `w`: set the transport debug level to warning
- `e`: set the transport debug level to error
- `s`: toggle silence
- `l`: toggle level labels
- `t`: toggle timestamps
- `p`: toggle elapsed-time profiler display
- `c`: toggle colours
- `filter <text>`: show output containing the supplied text
- `nofilter`: clear the filter
- `reset`: restart the ESP32 when explicitly enabled with `setResetCmdEnabled(true)`

Commands affect the transport from which they were issued where separate transport state exists. For example, `d` in the browser changes the WebSocket level and does not change the active Telnet level.

The WebSocket welcome message mirrors the Telnet welcome information and includes the library version, hostname, IP address, MAC address, free heap, ESP SDK version, supported commands, and configured project-command help.

## WebSocket API

Start the library-owned browser console and WebSocket endpoint with:

```cpp
bool beginWebSocket(
    AsyncWebServer* server,
    const String& hostName,
    const char* pagePath = "/debug");
```

The hostname argument allows WebSocket-only applications to provide the same identifying information shown to Telnet clients.

Useful WebSocket methods include:

- `beginWebSocket()`
- `setWebDebugLevel()` and `getWebDebugLevel()`
- `setWebSilenced()` and `isWebSilenced()`
- `showWebDebugLevel()` and `showWebProfiler()`
- `webClientCount()` and `hasWebClients()`
- `sendWebMessage()`
- `webBatchLength()` and `webBatchCapacity()`
- `droppedWebLines()` and `droppedWebBytes()`
- `flush()`

## Password protection

Configure a Telnet password before accepting clients:

```cpp
Debug.setPassword("change-me");
```

When `REMOTEDEBUG_HIDE_PASSWORD` is enabled, the library uses Telnet ECHO negotiation to ask a compliant client to suppress local echo while the password is entered. Normal echo is restored after authentication or disconnection.

This only hides the password on screen. **Telnet remains unencrypted**, so the password and all debug traffic can still be observed on the network.

The built-in WebSocket transport does not implement password authentication. If authentication, authorisation, or TLS is required, enforce it elsewhere in the application or network architecture before exposing the debug service.

## Compile-time configuration

Settings can be supplied as build flags or changed in `RemoteDebugCfg.h`. Prefer build flags for project-specific values so library updates do not overwrite local configuration.

### Example PlatformIO configuration

```ini
build_flags =
    -D REMOTEDEBUG_ENABLE_WEBSOCKET=1
    -D REMOTEDEBUG_WEB_MAX_LINE_LENGTH=512
    -D REMOTEDEBUG_WEB_MAX_BATCH_LENGTH=16384
    -D REMOTEDEBUG_MAX_COMMAND_LENGTH=256
    -D REMOTEDEBUG_PWD_ATTEMPTS=3
    -D REMOTEDEBUG_AUTH_BACKOFF_MS=1000UL
    -D REMOTEDEBUG_HIDE_PASSWORD=1
    -D REMOTEDEBUG_MAX_TIME_INACTIVE_MS=600000UL
    -D REMOTEDEBUG_PRINT_BUFFER_SIZE=256
    -D REMOTEDEBUG_SHOW_HELP=true
    -D REMOTEDEBUG_CLIENT_BUFFERING=true
```

### Configuration reference

| Variable | Default | Purpose |
|---|---:|---|
| `DEBUG_DISABLED` | Undefined | Compiles out the RemoteDebug implementation and turns logging macros into no-ops. |
| `REMOTEDEBUG_ENABLE_WEBSOCKET` | `0` | Enables `ESPAsyncWebServer` WebSocket support when set to `1`. |
| `REMOTEDEBUG_WEB_MAX_LINE_LENGTH` | `1024` | Maximum WebSocket line length before the current line is flushed. |
| `REMOTEDEBUG_WEB_MAX_BATCH_LENGTH` | `8192` | Maximum accumulated WebSocket batch length. Additional complete lines are dropped and counted when full. |
| `TELNET_PORT` | `23` | Default port used by the Telnet `begin()` overloads. |
| `REMOTEDEBUG_MAX_COMMAND_LENGTH` | `128` | Maximum accepted Telnet or WebSocket command length. |
| `DEBUG_DISABLE_AUTO_FUNC` | Undefined | Removes automatic function-name and ESP32-core prefixes from logging macros. |
| `REMOTEDEBUG_PWD_ATTEMPTS` | `3` | Incorrect Telnet password attempts permitted before disconnection. |
| `REMOTEDEBUG_AUTH_BACKOFF_MS` | `1000UL` | Base duration for increasing authentication delay after failed attempts. |
| `REMOTEDEBUG_HIDE_PASSWORD` | `1` | Uses Telnet ECHO negotiation to suppress local password display. This does not provide encryption. |
| `REMOTEDEBUG_MAX_TIME_INACTIVE_MS` | `600000UL` | Normal Telnet inactivity timeout. Set to `0` to disable it. |
| `MAX_TIME_INACTIVE` | Alias | Backward-compatible alias for `REMOTEDEBUG_MAX_TIME_INACTIVE_MS`. |
| `REMOTEDEBUG_PRINT_BUFFER_SIZE` | `150U` | Telnet and Serial line-buffer flush threshold. |
| `BUFFER_PRINT` | Alias | Backward-compatible alias for `REMOTEDEBUG_PRINT_BUFFER_SIZE`. |
| `REMOTEDEBUG_SHOW_HELP` | `true` | Shows initial command help to newly connected clients. |
| `SHOW_HELP` | Alias | Backward-compatible alias for `REMOTEDEBUG_SHOW_HELP`. |
| `REMOTEDEBUG_CLIENT_BUFFERING` | `true` | Buffers Telnet output to reduce frequent small writes. |
| `CLIENT_BUFFERING` | Derived alias | Defined for compatibility when client buffering is enabled. |
| `DELAY_TO_SEND` | `10` | Telnet buffered-send interval in milliseconds. |
| `MAX_SIZE_SEND` | `1460` | Maximum buffered Telnet send size. |
| `DEBUGGER_ENABLED` | `true` | Enables callback-based debugger integration. |
| `DEBUGGER_HANDLE_TIME` | `850` | Minimum interval between scheduled debugger callback handling. |
| `DEBUGGER_SEND_INFO` | Undefined | Reserved optional debugger-information support. |

## Buffering and delivery

Telnet buffering and WebSocket batching reduce the number of network writes, but debug output is not a guaranteed-delivery channel.

- Buffer capacity is finite.
- Slow clients can cause WebSocket lines or bytes to be dropped.
- Dropped-output counters report loss but cannot recover missing output.
- Avoid using debug output for application messaging or audit records.

Monitor `droppedWebLines()` and `droppedWebBytes()` when diagnosing missing WebSocket output.

## Security considerations

- Treat Telnet as plaintext even when a password is configured and hidden on screen.
- The built-in HTTP and WebSocket console is also plaintext unless protected by an external TLS termination design.
- Use remote debugging only on a trusted management network or through a VPN.
- Restrict access with firewall rules.
- Do not expose Telnet, HTTP, or WebSocket debugging directly to the public internet.
- Do not log passwords, tokens, private keys, personal information, or other sensitive content.
- Compile production firmware with `DEBUG_DISABLED` unless remote diagnostics are explicitly required and risk-assessed.

## Compatibility and troubleshooting

- Confirm that the installed library is this fork rather than another library with the same name.
- Confirm that the project targets ESP32.
- Call `Debug.handle()` regularly for every transport mode.
- Enable `REMOTEDEBUG_ENABLE_WEBSOCKET=1` before using WebSocket APIs.
- For WebSocket-only operation, pass the hostname to `beginWebSocket()`.
- Call `webServer.begin()` after registering the RemoteDebug WebSocket page.
- If a custom Telnet port is used, confirm that firewall rules and the client use that port.
- If password characters remain visible, confirm that `REMOTEDEBUG_HIDE_PASSWORD` is enabled and that the Telnet client supports ECHO negotiation.
- Ensure the ESP32 Arduino core, `ESPAsyncWebServer`, and asynchronous TCP dependency are mutually compatible.
- Test Telnet and WebSocket independently before testing both together.
- Perform a clean rebuild after changing public method signatures or compile-time settings.

## Examples

Examples may vary between revisions. Check the source under `examples/` and compare it with the installed `RemoteDebug.h`.

Common example names include:

- `RemoteDebug_Basic`: basic setup and logging
- `RemoteDebug_Advanced`: commands, filters, display options, and profiler behaviour
- `RemoteDebug_Debugger`: debugger callback integration
- `DualTransport`: Telnet and WebSocket integration
- `WebSocketDebug`: WebSocket integration

## Contributing and issues

When reporting an issue, include:

- Board model and ESP32 Arduino core version
- Library revision or commit
- `ESPAsyncWebServer` and asynchronous TCP dependency versions, if applicable
- Relevant compile-time flags
- A minimal reproducer
- Compiler output or diagnostic logs

## Upstream projects

- [JoaoLopesF/RemoteDebug](https://github.com/JoaoLopesF/RemoteDebug)
- [ktos/RemoteDebug](https://github.com/ktos/RemoteDebug)
