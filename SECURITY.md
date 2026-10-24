# Security guidance

RemoteDebug is a development diagnostics library. Its Telnet transport is plaintext: credentials, commands, device metadata, and log content can be observed or modified by an attacker on the network path.

- Do not expose the Telnet listener to the internet, guest Wi-Fi, or another untrusted network.
- Prefer an isolated management network or a VPN and restrict access with a firewall.
- Password attempts are rate-limited globally, but the password is not a substitute for encrypted transport.
- Do not emit secrets, credentials, access tokens, or personal information in debug messages.
- Compile production firmware with `DEBUG_DISABLED` unless remote diagnostics are explicitly required and risk-assessed.

## Concurrency model

A `RemoteDebug` instance is single-task. Call `handle()`, the debug macros, and direct `Print` methods from the same Arduino task. Calls from multiple ESP32 tasks are unsupported because message framing and transport routing are stateful. If multi-task logging is needed, send records to one logging task through a bounded FreeRTOS queue.
