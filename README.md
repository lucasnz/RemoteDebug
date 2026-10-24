# RemoteDebug

**Remote debugging for ESP32 over Telnet and optional WebSocket transports, using one logging API.**

This project is a fork of [JoaoLopesF/RemoteDebug](https://github.com/JoaoLopesF/RemoteDebug) and incorporates changes from [ktos/RemoteDebug](https://github.com/ktos/RemoteDebug).

- **This fork:** <https://github.com/lucasnz/RemoteDebug>
- **Original upstream:** <https://github.com/JoaoLopesF/RemoteDebug>
- **Additional source:** <https://github.com/ktos/RemoteDebug>

## Features

- ESP32 support using the Arduino framework.
- Telnet logging on a configurable runtime port.
- Optional `ESPAsyncWebServer` WebSocket output.
- Independent Telnet and WebSocket debug levels.
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

For WebSocket support, also add a compatible asynchronous web server:

```ini
lib_deps =
    https://github.com/lucasnz/RemoteDebug.git
    ESP32Async/ESPAsyncWebServer
```

Avoid installing conflicting versions of `ESPAsyncWebServer` or its asynchronous TCP dependency.

### Arduino IDE

1. Download or clone this repository.
2. Install it in the Arduino libraries directory.
3. If WebSocket support is required, install a compatible `ESPAsyncWebServer` and its dependencies.
4. Include `RemoteDebug.h` in the sketch.

## Basic Telnet usage

```cpp
#include <WiFi.h>
#include <RemoteDebug.h>

RemoteDebug Debug;

void setup()
{
    Serial.begin(115200);

    // Connect Wi-Fi before starting RemoteDebug.
    // WiFi.begin(...);

    Debug.begin("my-device");
    debugI("RemoteDebug started");
}

void loop()
{
    Debug.handle();
    debugD("Example debug message");
}
```

Call `Debug.handle()` regularly. It accepts Telnet connections, processes commands, handles timeouts, flushes buffered output, and services the optional WebSocket transport.

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

`begin()` returns `false` if the port is zero, the level is invalid, or the server cannot be created.

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

1. `VERBOSE`
2. `DEBUG`
3. `INFO`
4. `WARNING`
5. `ERROR`
6. `ANY`

The logging macros include the current function name and ESP32 core ID unless automatic function prefixes are disabled.

The library is designed for a single application task. Do not call `handle()`, logging macros, or direct `Print` methods concurrently from multiple tasks without providing external serialisation.

## Telnet commands

The main built-in commands are:

- `?` or `h`: show command help
- `q`: close the Telnet session
- `m`: show free heap
- `v`, `d`, `i`, `w`, `e`: select the Telnet debug level
- `s`: toggle silence
- `l`: toggle level labels
- `t`: toggle timestamps
- `p`: toggle elapsed-time profiler display
- `c`: toggle ANSI colours
- `filter <text>`: show matching output
- `nofilter`: clear the filter
- `reset`: restart the ESP32 when explicitly enabled with `setResetCmdEnabled(true)`

## Password protection

Configure a Telnet password before accepting clients:

```cpp
Debug.setPassword("change-me");
```

When `REMOTEDEBUG_HIDE_PASSWORD` is enabled, the library uses Telnet ECHO negotiation to ask a compliant client to suppress local echo while the password is entered. Normal echo is restored after authentication or disconnection.

This only hides the password on screen. **Telnet remains unencrypted**, so the password and all debug traffic can still be observed on the network.

Failed password attempts use a global increasing delay. The retry limit and base backoff are configurable.

## WebSocket usage

WebSocket support must be enabled at compile time. The application owns the `AsyncWebSocket`, web server, authentication, authorisation, and TLS configuration.

```cpp
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <RemoteDebug.h>

AsyncWebServer server(80);
AsyncWebSocket debugSocket("/debug");
RemoteDebug Debug;

void setup()
{
    Serial.begin(115200);

    // Connect Wi-Fi before starting the server and RemoteDebug.

    Debug.begin("my-device", RemoteDebug::DEBUG);

    server.addHandler(&debugSocket);
    server.begin();

    Debug.attachWebSocket(&debugSocket);

    // Only do this after the application has authenticated and authorised
    // access to the WebSocket endpoint.
    Debug.setWebAccessAuthorized(true);

    Debug.setWebDebugLevel(RemoteDebug::INFO);
    debugI("WebSocket debugging started");
}

void loop()
{
    Debug.handle();
}
```

Useful WebSocket methods include:

- `attachWebSocket()` and `detachWebSocket()`
- `setWebDebugLevel()` and `getWebDebugLevel()`
- `setWebSilenced()` and `isWebSilenced()`
- `showWebDebugLevel()` and `showWebProfiler()`
- `webClientCount()` and `hasWebClients()`
- `sendWebMessage()`
- `webBatchLength()` and `webBatchCapacity()`
- `droppedWebLines()` and `droppedWebBytes()`
- `flush()`

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
| `DEBUG_DISABLED` | Undefined | Compiles out the `RemoteDebug` class implementation and turns logging macros into no-ops. Define it for production builds that do not require remote debugging. |
| `REMOTEDEBUG_ENABLE_WEBSOCKET` | `0` | Enables optional `ESPAsyncWebServer` WebSocket support when set to `1`. See the conditional-compilation note below. |
| `REMOTEDEBUG_WEB_MAX_LINE_LENGTH` | `1024` | Maximum WebSocket line length in bytes before the current line is flushed. |
| `REMOTEDEBUG_WEB_MAX_BATCH_LENGTH` | `8192` | Maximum accumulated WebSocket batch length in bytes. Additional complete lines are dropped and counted when the batch is full. |
| `TELNET_PORT` | `23` | Default port used by `begin(hostName)` and `begin(hostName, level)`. A different port can be passed directly to `begin(hostName, port, level)`. |
| `REMOTEDEBUG_MAX_COMMAND_LENGTH` | `128` | Maximum accepted printable Telnet command length. Extra characters are discarded until the line terminator. |
| `DEBUG_DISABLE_AUTO_FUNC` | Undefined | Removes the automatic function-name and ESP32 core prefix from logging macros. |
| `REMOTEDEBUG_PWD_ATTEMPTS` | `3` | Number of incorrect password attempts permitted before the Telnet client is disconnected. |
| `REMOTEDEBUG_AUTH_BACKOFF_MS` | `1000UL` | Base duration for increasing authentication backoff after failed password attempts. |
| `REMOTEDEBUG_HIDE_PASSWORD` | `1` | Uses Telnet ECHO negotiation to suppress local password display in compliant clients. This does not encrypt the password. Set to `0` to disable negotiation. |
| `REMOTEDEBUG_MAX_TIME_INACTIVE_MS` | `600000UL` | Normal Telnet inactivity timeout in milliseconds. The default is 10 minutes. Set to `0` to disable the normal inactivity timeout. |
| `MAX_TIME_INACTIVE` | Alias | Backward-compatible alias for `REMOTEDEBUG_MAX_TIME_INACTIVE_MS`. Prefer the prefixed variable in new projects. |
| `REMOTEDEBUG_PRINT_BUFFER_SIZE` | `150U` | Telnet/Serial line-buffer flush threshold in bytes. Long output is flushed when the threshold is reached. |
| `BUFFER_PRINT` | Alias | Backward-compatible alias for `REMOTEDEBUG_PRINT_BUFFER_SIZE`. Prefer the prefixed variable in new projects. |
| `REMOTEDEBUG_SHOW_HELP` | `true` | Shows the initial Telnet help text after connection and successful authentication. |
| `SHOW_HELP` | Alias | Backward-compatible alias for `REMOTEDEBUG_SHOW_HELP`. Prefer the prefixed variable in new projects. |
| `REMOTEDEBUG_CLIENT_BUFFERING` | `true` | Buffers Telnet output to reduce the overhead and latency effects of frequent small writes. |
| `CLIENT_BUFFERING` | Derived alias | Defined for backward compatibility when `REMOTEDEBUG_CLIENT_BUFFERING` is enabled. |
| `DELAY_TO_SEND` | `10` | Telnet output-buffer send interval in milliseconds when client buffering is enabled. |
| `MAX_SIZE_SEND` | `1460` | Maximum buffered Telnet send size in bytes. The default aligns with a typical TCP maximum segment payload. |
| `DEBUGGER_ENABLED` | `true` | Compiles callback-based debugger integration. In the current configuration file this is defined unconditionally; disable it by editing the file or making the definition conditional. |
| `DEBUGGER_HANDLE_TIME` | `850` | Minimum interval in milliseconds between scheduled debugger callback handling. |
| `DEBUGGER_SEND_INFO` | Undefined | Reserved optional debugger-information support. It is currently commented out and the configuration notes indicate the app does not yet expose it. |

## Buffering and delivery

Telnet buffering and WebSocket batching reduce the number of network writes, but debug output is not a guaranteed-delivery channel.

- Buffer capacity is finite.
- Slow clients can cause WebSocket lines or bytes to be dropped.
- Dropped-output counters report loss but cannot recover missing output.
- Avoid using debug output for application messaging or audit records.

Monitor `droppedWebLines()` and `droppedWebBytes()` when diagnosing missing WebSocket output.

## Security considerations

- Treat Telnet as plaintext even when a password is configured and hidden on screen.
- Use remote debugging only on a trusted management network or through a VPN.
- Restrict access with firewall rules.
- Do not expose Telnet or WebSocket debugging directly to the public internet.
- Do not log passwords, tokens, private keys, personal information, or other sensitive content.
- Compile production firmware with `DEBUG_DISABLED` unless remote diagnostics are explicitly required and risk-assessed.

## Compatibility and troubleshooting

- Confirm that the installed library is this fork rather than another library with the same name.
- Confirm that the project targets ESP32.
- Call `Debug.handle()` regularly.
- If a custom Telnet port is used, confirm that firewall rules and the client use that port.
- If password characters remain visible, confirm that `REMOTEDEBUG_HIDE_PASSWORD` is enabled and that the Telnet client supports ECHO negotiation.
- If WebSocket code compiles unexpectedly, review the conditional-compilation note above.
- For WebSocket output, confirm that the endpoint is registered, attached, and explicitly authorised.
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

## Contributing and issues

Submit issues and changes specific to this fork at:

<https://github.com/lucasnz/RemoteDebug>

Include:

- Board model and ESP32 Arduino core version
- Library revision or commit
- `ESPAsyncWebServer` and asynchronous TCP dependency versions, if applicable
- Relevant compile-time flags
- A minimal reproducer
- Compiler output or diagnostic logs

## Upstream projects

- <https://github.com/JoaoLopesF/RemoteDebug>
- <https://github.com/ktos/RemoteDebug>
