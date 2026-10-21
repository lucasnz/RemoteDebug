# RemoteDebug

**An ESP32 remote debugging library with integrated Telnet and WebSocket transports.**

This project is a fork of [JoaoLopesF/RemoteDebug](https://github.com/JoaoLopesF/RemoteDebug) and incorporates changes from [ktos/RemoteDebug](https://github.com/ktos/RemoteDebug). The goal of the project is to provide **one library, one logging API, and multiple transports**. The library provides Telnet and WebSocket based `RemoteDebug` support.

- **Fork:** https://github.com/lucasnz/RemoteDebug
- **Upstream:** https://github.com/JoaoLopesF/RemoteDebug
- **Incorporates** https://github.com/ktos/RemoteDebug

## Features

### Integrated dual-transport debugging

This fork brings together the original RemoteDebug functionality and WebSocket-based debugging.

- **Telnet debugging:** Retains the familiar remote logging workflow from the upstream library.
- **WebSocket debugging:** Adds a WebSocket transport for delivering diagnostic output to browser-based clients.
- **Unified logging interface:** Uses the `RemoteDebug` class and existing logging macros rather than requiring a separate `WebRemoteDebug` object.
- **Shared library:** Keeps transport integration and debugging configuration within one library.

### WebSocket-specific functionality

The WebSocket integration adds controls for managing browser-based debugging output, including:

- Attaching and detaching a WebSocket endpoint.
- Configuring the WebSocket debugging level independently.
- Enabling or disabling WebSocket output.
- Controlling the display of debugging levels and profiler information.
- Checking whether WebSocket clients are connected and obtaining the client count.
- Sending messages to WebSocket clients.
- Inspecting output batching and buffer statistics.
- Monitoring dropped WebSocket lines and bytes.

The exact API is defined by the headers in the version installed. See `src/RemoteDebug.h` for the available methods.

### RemoteDebug functionality

The fork retains the original library's core debugging capabilities, including:

- Multiple logging levels:
  - `PROFILER`
  - `VERBOSE`
  - `DEBUG`
  - `INFO`
  - `WARNING`
  - `ERROR`
  - `ANY`
- Logging through the familiar `debugV()`, `debugD()`, `debugI()`, `debugW()`, and `debugE()` macros.
- Remote debugging session management.
- Optional password configuration.
- Configurable output colours and timestamps.
- Debug-level display.
- Profiler support and automatic profiler-level adjustment.
- Project-specific commands, command help, and callbacks.
- Optional reset commands.
- New-client and debugger callbacks.
- Optional serial output.

Refer to the source and examples for the precise behaviour and supported configuration options.

## Platform and dependencies

This fork is intended for ESP32 projects using the Arduino framework.

### Requirements

- An ESP32-compatible Arduino environment.
- A compatible `ESPAsyncWebServer` installation for WebSocket support.
- The asynchronous TCP dependency required by the selected web-server version.
- A network connection for remote debugging.

The WebSocket transport depends on the asynchronous web-server stack. Ensure the versions of the ESP32 Arduino core, `ESPAsyncWebServer`, and its dependencies are compatible with one another.

The original upstream library supported additional environments. Do not assume ESP8266 compatibility with this fork unless it has been explicitly tested for the revision you are using.

## Installation

### PlatformIO

Add the fork to your project's `platformio.ini` dependencies:

```ini
lib_deps =
    https://github.com/lucasnz/RemoteDebug.git
    ESP32Async/ESPAsyncWebServer
```

Check the dependency requirements for your selected `ESPAsyncWebServer` release and add any required asynchronous TCP dependency if it is not installed automatically.

If your project already uses `ESPAsyncWebServer`, avoid introducing conflicting versions of the same dependency.

### Arduino IDE

1. Download or clone this repository.
2. Install the library in your Arduino libraries directory.
3. Install a compatible `ESPAsyncWebServer` dependency and its required dependencies.
4. Include `RemoteDebug.h` in your sketch.
5. Configure the debugging transports required by your application.

## Basic usage

The following illustrates the general integration pattern. Adapt the Wi-Fi setup, WebSocket registration, and method calls to the API available in your checked-out revision.

```cpp
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <RemoteDebug.h>

AsyncWebServer server(80);
AsyncWebSocket debugSocket("/debug");

RemoteDebug Debug;

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi before starting remote debugging.
  // Configure WiFi.begin(...) and wait for the connection.

  Debug.begin("my-device", RemoteDebug::DEBUG);

  // Register the WebSocket endpoint with the web server.
  server.addHandler(&debugSocket);
  server.begin();

  // Attach the WebSocket transport to RemoteDebug.
  Debug.attachWebSocket(&debugSocket);

  Debug.setResetCmdEnabled(true);

  debugI("RemoteDebug started");
}

void loop() {
  // Service the RemoteDebug library regularly.
  Debug.handle();

  debugD("Example debug message");
}
```

This example demonstrates the intended arrangement of the components; it is not a substitute for checking the exact method signatures in `src/RemoteDebug.h`. The application must also provide a compatible WebSocket client if it needs to display the output in a browser.

### Logging messages

Use the standard RemoteDebug logging macros:

```cpp
debugV("Verbose message");
debugD("Debug message");
debugI("Informational message");
debugW("Warning message");
debugE("Error message");
```

Call `Debug.handle()` regularly from the Arduino `loop()` so the library can perform its normal processing.

## Examples

See the `examples/` directory for usage patterns included in the repository.

| Example | Purpose |
| --- | --- |
| `RemoteDebug_Basic` | Basic setup and remote logging. |
| `RemoteDebug_Advanced` | Advanced configuration, commands, and profiler functionality. |
| `RemoteDebug_Debugger` | Debugger and command integration. |
| `DualTransport` | Demonstrates integration of the WebSocket transport alongside the original debugging transport. |

Example availability and implementation details may vary between revisions. Check each example's comments and the library header before relying on a particular method or behaviour.

## Buffering and output delivery

WebSocket output may be buffered or batched to reduce the overhead of sending frequent log messages.

The library exposes diagnostics for inspecting WebSocket buffering and dropped output. These are useful when investigating missing messages or a slow browser client.

Keep in mind:

- Buffer capacity is finite.
- A slow or disconnected client may not receive every message.
- Dropped-output counters indicate that output has been lost; they do not recover missing messages.
- Debugging output should not be used as a guaranteed-delivery messaging mechanism.

If you need to diagnose output loss, monitor the available batch, buffer, and dropped-output statistics in your version of the library.

## Security considerations

Remote debugging can expose internal application state and, depending on the configuration, debugging commands.

- Use remote debugging only on a trusted network or behind a VPN.
- Do not expose debugging endpoints directly to the public internet.
- Use network firewall rules to restrict access.
- Treat Telnet as an unencrypted transport, even when password protection is enabled.
- Avoid logging passwords, tokens, or other sensitive information.

A password alone does not make an unencrypted debugging connection secure.

## Compatibility and troubleshooting

If the library does not compile or the WebSocket transport does not behave as expected, check the following:

1. Confirm that the installed library is this fork, rather than another copy of `RemoteDebug`.
2. Check that the ESP32 Arduino core and asynchronous web-server dependencies are compatible.
3. Confirm that the WebSocket endpoint is registered with the web server.
4. Confirm that the WebSocket transport is attached using the method supported by the installed revision.
5. Ensure `Debug.handle()` is called regularly.
6. Check that the browser client connects to the configured WebSocket path.
7. Inspect the compiler output for API differences between examples and the installed source.
8. Test Telnet and WebSocket output separately before testing them together.

For reproducible results, build the examples against the exact versions of the library and dependencies used by your application.

## Contributing and issues

Issues and contributions specific to this fork should be submitted through: [github.com/lucasnz/RemoteDebug](https://github.com/lucasnz/RemoteDebug)

When reporting an issue, include:

- The board and ESP32 Arduino core version.
- The library revision or commit.
- The versions of `ESPAsyncWebServer` and its dependencies.
- A minimal example that reproduces the issue.
- Relevant compiler output or debugging logs.

## Upstream project

This project is derived from: [ktos/RemoteDebug](https://github.com/ktos/RemoteDebug), which builds on [JoaoLopesF/RemoteDebug]((https://github.com/JoaoLopesF/RemoteDebug)

Please refer to the upstream repository for the history and original implementation of RemoteDebug. Changes and extensions specific to this fork are maintained in: [lucasnz/RemoteDebug](https://github.com/lucasnz/RemoteDebug)
