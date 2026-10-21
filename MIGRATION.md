# Migrating to RemoteDebug 4.0.0

- Remove `WebRemoteDebug.h` and its separate `WebRemoteDebug` object.
- Keep one `RemoteDebug Debug` instance.
- Ensure `REMOTEDEBUG_ENABLE_WEBSOCKET` is enabled in `src/RemoteDebugCfg.h` while the library is compiled.
- Install ESPAsyncWebServer and the correct async TCP dependency for the selected MCU.
- After `Debug.begin(...)`, call `Debug.attachWebSocket(&debugSocket)`. Register the socket with the app's `AsyncWebServer` and call `Debug.handle()` in `loop()`.
- Replace `Debug.remote().method()` with `Debug.method()` because the merged object is the logger itself.
- WebSocket command parsing remains application-specific. Use the socket event callback and validate commands before invoking application actions.
- To return to a Telnet-only build, disable `REMOTEDEBUG_ENABLE_WEBSOCKET` in `RemoteDebugCfg.h` and remove the async web-server dependency.
