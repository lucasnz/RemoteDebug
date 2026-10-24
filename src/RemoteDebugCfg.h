/*
 * All configurations have moved to RemoteDebugCfg.h,
 * to facilitate changes
 */

///////////// User config, to not lost in library updates

#ifndef REMOTEDEBUGCFG_H_
#define REMOTEDEBUGCFG_H_
#pragma once

///////////// For RemoteDebug ///////////////////

///// Debug disable for compile to production/release
///// as nothing of RemotedDebug is compiled, zero overhead :-)
//#define DEBUG_DISABLED true

// Optional AsyncWebServer WebSocket output transport. Define this before
// Define in platformio.ini build flags to enable:
// -D REMOTEDEBUG_ENABLE_WEBSOCKET=1
//#define REMOTEDEBUG_ENABLE_WEBSOCKET

// Override these in platformio.ini build flags:
// -D REMOTEDEBUG_WEB_MAX_LINE_LENGTH=512
// -D REMOTEDEBUG_WEB_MAX_BATCH_LENGTH=16384
#ifndef REMOTEDEBUG_WEB_MAX_LINE_LENGTH
    #define REMOTEDEBUG_WEB_MAX_LINE_LENGTH 1024
#endif
#ifndef REMOTEDEBUG_WEB_MAX_BATCH_LENGTH
    #define REMOTEDEBUG_WEB_MAX_BATCH_LENGTH 8192
#endif

#ifndef DEBUG_DISABLED

///// Port for telnet server
#define TELNET_PORT 23

// Maximum accepted Telnet command length (overlong commands are discarded).
#define REMOTEDEBUG_MAX_COMMAND_LENGTH 128

// Disable auto function for debug macros? (uncomment this if not want this)
//#define DEBUG_DISABLE_AUTO_FUNC true

// Simple password request - left commented if not need this - 18/07/18
// Notes:
// It is very simple feature, only text, no cryptography,
// and the password is echoed in screen (I not discovery yet how disable it)
// telnet use advanced authentication (kerberos, etc.)
// Such now as RemoteDebug now is not for production releases,
// this kind of authentication will not be done now.
// Can be by project, just call setPassword method
#ifndef REMOTEDEBUG_PWD_ATTEMPTS
#define REMOTEDEBUG_PWD_ATTEMPTS 3
#endif
#ifndef REMOTEDEBUG_AUTH_BACKOFF_MS
#define REMOTEDEBUG_AUTH_BACKOFF_MS 1000UL
#endif

// Ask Telnet clients to disable local echo while a password is entered.
// This hides the password on screen but does not encrypt the connection.
#ifndef REMOTEDEBUG_HIDE_PASSWORD
#define REMOTEDEBUG_HIDE_PASSWORD 1
#endif

// Maximum time for inactivity (em milliseconds)
// Default: 10 minutes
// Comment it if you not want this
// Can be by project, just define it before include this file
#ifndef REMOTEDEBUG_MAX_TIME_INACTIVE_MS
#define REMOTEDEBUG_MAX_TIME_INACTIVE_MS 600000UL
#endif
#ifndef MAX_TIME_INACTIVE
#define MAX_TIME_INACTIVE REMOTEDEBUG_MAX_TIME_INACTIVE_MS // backwards compatibility
#endif

// Buffered print write to WiFi -> length of buffer
// Can be by project, just define it before include this file
#ifndef REMOTEDEBUG_PRINT_BUFFER_SIZE
#define REMOTEDEBUG_PRINT_BUFFER_SIZE 150U
#endif
#ifndef BUFFER_PRINT
#define BUFFER_PRINT REMOTEDEBUG_PRINT_BUFFER_SIZE // backwards compatibility
#endif

// Should the help text be displayed on connection.
// Enabled by default, comment to disable
#ifndef REMOTEDEBUG_SHOW_HELP
#define REMOTEDEBUG_SHOW_HELP true
#endif
#ifndef SHOW_HELP
#define SHOW_HELP REMOTEDEBUG_SHOW_HELP // backwards compatibility
#endif

// Buffering (sends in interval of time to avoid ESP misterious delays)
// Uncomment this to disable it
#ifndef REMOTEDEBUG_CLIENT_BUFFERING
#define REMOTEDEBUG_CLIENT_BUFFERING true
#endif
#if REMOTEDEBUG_CLIENT_BUFFERING
#define CLIENT_BUFFERING true // backwards compatibility
#endif
#ifdef CLIENT_BUFFERING
#define DELAY_TO_SEND 10 // Time to send buffer
#define MAX_SIZE_SEND 1460 // Maximum size of packet (limit of TCP/IP)
#endif

// Debugger support enabled ?
// Comment this to disable it
#define DEBUGGER_ENABLED true
#ifdef DEBUGGER_ENABLED
#define DEBUGGER_HANDLE_TIME 850 // Interval to call handle of debugger - equal to implemented in debugger
// App have debugger elements on screen ?
// Note: app not have it yet
//#define DEBUGGER_SEND_INFO true
#endif

#endif /* DEBUG_DISABLED */

#endif /* REMOTEDEBUGCFG_H_ */

////// End
