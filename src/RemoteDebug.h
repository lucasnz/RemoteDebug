#ifndef REMOTEDEBUG_H
#define REMOTEDEBUG_H
#pragma once

///// RemoteDebug configuration

#include "RemoteDebugCfg.h"

// Debug enabled ?

#ifndef DEBUG_DISABLED

//////// Defines

// New color system (comment this to return to old system) - 2019-02-27

#define COLOR_NEW_SYSTEM true

// ANSI Colors

#define COLOR_RESET "\x1B[0m"

#define COLOR_BLACK "\x1B[0;30m"
#define COLOR_RED "\x1B[0;31m"
#define COLOR_GREEN "\x1B[0;32m"
#define COLOR_YELLOW "\x1B[0;33m"
#define COLOR_BLUE "\x1B[0;34m"
#define COLOR_MAGENTA "\x1B[0;35m"
#define COLOR_CYAN "\x1B[0;36m"
#define COLOR_WHITE "\x1B[0;37m"

#define COLOR_DARK_BLACK "\x1B[1;30m"

#define COLOR_LIGHT_RED "\x1B[1;31m"
#define COLOR_LIGHT_GREEN "\x1B[1;32m"
#define COLOR_LIGHT_YELLOW "\x1B[1;33m"
#define COLOR_LIGHT_BLUE "\x1B[1;34m"
#define COLOR_LIGHT_MAGENTA "\x1B[1;35m"
#define COLOR_LIGHT_CYAN "\x1B[1;36m"
#define COLOR_LIGHT_WHITE "\x1B[1;37m"

#define COLOR_BACKGROUND_BLACK "\x1B[40m"
#define COLOR_BACKGROUND_RED "\x1B[41m"
#define COLOR_BACKGROUND_GREEN "\x1B[42m"
#define COLOR_BACKGROUND_YELLOW "\x1B[43m"
#define COLOR_BACKGROUND_BLUE "\x1B[44m"
#define COLOR_BACKGROUND_MAGENTA "\x1B[45m"
#define COLOR_BACKGROUND_CYAN "\x1B[46m"
#define COLOR_BACKGROUND_WHITE "\x1B[47m"

#ifdef COLOR_NEW_SYSTEM
// New system of Colors
// Note: this colors is not equals to SerialDebug colors, due using standard 16 colors of Ansi, for compatibility
#define COLOR_VERBOSE 	COLOR_GREEN
#define COLOR_DEBUG		COLOR_LIGHT_GREEN
//#define COLOR_INFO		COLOR_YELLOW
//#define COLOR_WARNING	COLOR_CYAN
//#define COLOR_ERROR		COLOR_RED
#define COLOR_INFO		COLOR_LIGHT_YELLOW
#define COLOR_WARNING	COLOR_LIGHT_CYAN
#define COLOR_ERROR		COLOR_LIGHT_RED
#define COLOR_RAW		COLOR_WHITE // COLOR_MAGENTA
#endif

//////// Includes

#include "Arduino.h"
#include "Print.h"

#if REMOTEDEBUG_ENABLE_WEBSOCKET && !defined(DEBUG_DISABLED)
#include <ESPAsyncWebServer.h>
#endif

// This library targets ESP32 only.
#if !defined(ESP32)
#error "RemoteDebug supports ESP32 only"
#endif
#include <WiFi.h>

////// Shortcut macros
#if defined(ESP32) && !defined(DEBUG_DISABLE_AUTO_FUNC)
#define DEBUG_AUTO_CORE true
#endif
// Statement-safe wrappers avoid dangling-else bugs. beginMessage() captures the
// level and transport routing for the immediately following Print call.
#ifdef DEBUG_AUTO_CORE
#define REMOTEDEBUG_PRINTF(fmt, ...) \
    Debug.printf("(%s)(C%d) " fmt, __func__, xPortGetCoreID(), ##__VA_ARGS__)
#elif !defined(DEBUG_DISABLE_AUTO_FUNC)
#define REMOTEDEBUG_PRINTF(fmt, ...) \
    Debug.printf("(%s) " fmt, __func__, ##__VA_ARGS__)
#else
#define REMOTEDEBUG_PRINTF(fmt, ...) Debug.printf(fmt, ##__VA_ARGS__)
#endif

#define REMOTEDEBUG_LOG(level, fmt, ...)             \
    do {                                             \
        if (Debug.beginMessage(level)) {             \
            REMOTEDEBUG_PRINTF(fmt, ##__VA_ARGS__);  \
            Debug.endMessage();                      \
        }                                            \
    } while (0)

#define rdebugA(fmt, ...) REMOTEDEBUG_LOG(Debug.ANY, fmt, ##__VA_ARGS__)
#define rdebugP(fmt, ...) REMOTEDEBUG_LOG(Debug.PROFILER, fmt, ##__VA_ARGS__)
#define rdebugV(fmt, ...) REMOTEDEBUG_LOG(Debug.VERBOSE, fmt, ##__VA_ARGS__)
#define rdebugD(fmt, ...) REMOTEDEBUG_LOG(Debug.DEBUG, fmt, ##__VA_ARGS__)
#define rdebugI(fmt, ...) REMOTEDEBUG_LOG(Debug.INFO, fmt, ##__VA_ARGS__)
#define rdebugW(fmt, ...) REMOTEDEBUG_LOG(Debug.WARNING, fmt, ##__VA_ARGS__)
#define rdebugE(fmt, ...) REMOTEDEBUG_LOG(Debug.ERROR, fmt, ##__VA_ARGS__)
// With newline

#define rdebugAln(fmt, ...) rdebugA(fmt "\n", ##__VA_ARGS__)
#define rdebugPln(fmt, ...) rdebugP(fmt "\n", ##__VA_ARGS__)
#define rdebugVln(fmt, ...) rdebugV(fmt "\n", ##__VA_ARGS__)
#define rdebugDln(fmt, ...) rdebugD(fmt "\n", ##__VA_ARGS__)
#define rdebugIln(fmt, ...) rdebugI(fmt "\n", ##__VA_ARGS__)
#define rdebugWln(fmt, ...) rdebugW(fmt "\n", ##__VA_ARGS__)
#define rdebugEln(fmt, ...) rdebugE(fmt "\n", ##__VA_ARGS__)

// For old versions compatibility

#define rdebug(fmt, ...) 	rdebugA(fmt, ##__VA_ARGS__)

// Another way - for compatibility

#define DEBUG(fmt, ...)   	rdebugA(fmt, ##__VA_ARGS__)

#define DEBUG_A(fmt, ...) 	rdebugA(fmt, ##__VA_ARGS__)
#define DEBUG_P(fmt, ...) 	rdebugP(fmt, ##__VA_ARGS__)
#define DEBUG_V(fmt, ...) 	rdebugV(fmt, ##__VA_ARGS__)
#define DEBUG_D(fmt, ...) 	rdebugD(fmt, ##__VA_ARGS__)
#define DEBUG_I(fmt, ...) 	rdebugI(fmt, ##__VA_ARGS__)
#define DEBUG_W(fmt, ...) 	rdebugW(fmt, ##__VA_ARGS__)
#define DEBUG_E(fmt, ...) 	rdebugE(fmt, ##__VA_ARGS__)

// New way: To compatibility with SerialDebug (can use RemoteDebug or SerialDebug)
// This is my favorite :)

#define debugV(fmt, ...) rdebugVln(fmt, ##__VA_ARGS__)
#define debugD(fmt, ...) rdebugDln(fmt, ##__VA_ARGS__)
#define debugI(fmt, ...) rdebugIln(fmt, ##__VA_ARGS__)
#define debugW(fmt, ...) rdebugWln(fmt, ##__VA_ARGS__)
#define debugE(fmt, ...) rdebugEln(fmt, ##__VA_ARGS__)
#define debugA(fmt, ...) rdebugAln(fmt, ##__VA_ARGS__)

#define debugHandle() Debug.handle()

// Macros used by code converter for codes with several prints to only message
// due the converter cannot
// convert severals Serial.print in one debug* macro.

#define REMOTEDEBUG_PRINT(level, method, x, ...)       \
    do {                                               \
        if (Debug.beginMessage(level)) {               \
            Debug.method(x, ##__VA_ARGS__);            \
            Debug.endMessage();                        \
        }                                              \
    } while (0)

#define rprintV(x, ...) REMOTEDEBUG_PRINT(Debug.VERBOSE, print, x, ##__VA_ARGS__)
#define rprintD(x, ...) REMOTEDEBUG_PRINT(Debug.DEBUG, print, x, ##__VA_ARGS__)
#define rprintI(x, ...) REMOTEDEBUG_PRINT(Debug.INFO, print, x, ##__VA_ARGS__)
#define rprintW(x, ...) REMOTEDEBUG_PRINT(Debug.WARNING, print, x, ##__VA_ARGS__)
#define rprintE(x, ...) REMOTEDEBUG_PRINT(Debug.ERROR, print, x, ##__VA_ARGS__)
#define rprintA(x, ...) REMOTEDEBUG_PRINT(Debug.ANY, print, x, ##__VA_ARGS__)

#define rprintVln(x, ...) REMOTEDEBUG_PRINT(Debug.VERBOSE, println, x, ##__VA_ARGS__)
#define rprintDln(x, ...) REMOTEDEBUG_PRINT(Debug.DEBUG, println, x, ##__VA_ARGS__)
#define rprintIln(x, ...) REMOTEDEBUG_PRINT(Debug.INFO, println, x, ##__VA_ARGS__)
#define rprintWln(x, ...) REMOTEDEBUG_PRINT(Debug.WARNING, println, x, ##__VA_ARGS__)
#define rprintEln(x, ...) REMOTEDEBUG_PRINT(Debug.ERROR, println, x, ##__VA_ARGS__)
#define rprintAln(x, ...) REMOTEDEBUG_PRINT(Debug.ANY, println, x, ##__VA_ARGS__)

///// Class

class RemoteDebug: public Print
{
	public:

	// Constructor

	RemoteDebug();

	// Methods

	bool begin(const String& hostName, uint16_t port, uint8_t startingDebugLevel = DEBUG);
	bool begin(const String& hostName, uint8_t startingDebugLevel = DEBUG);

	void setPassword(const String& password);

	void stop();

	void handle();

	void disconnect(boolean onlyTelnetClient = false);

	void setSerialEnabled(boolean enable);

	void setResetCmdEnabled(boolean enable);

	void setHelpProjectsCmds(const String& help);
	void setCallBackProjectCmds(void (*callback)());
	String getLastCommand();
	void clearLastCommand();

	void showTime(boolean show);
	void showProfiler(boolean show, uint32_t minTime = 0);
	void showDebugLevel(boolean show);
	void showColors(boolean show);

	void showRaw(boolean show);

	void setCallBackNewClient(void (*callback)());


	void setFilter(const String& filter);
	void setNoFilter();

	boolean isActive(uint8_t debugLevel = DEBUG) const;
	// Captures routing state for one logical message. endMessage() releases it.
	boolean beginMessage(uint8_t debugLevel = DEBUG);
	void endMessage();

#if REMOTEDEBUG_ENABLE_WEBSOCKET
    // Creates the WebSocket endpoint and serves the self-contained browser console.
    // The library owns the socket; the application continues to own the server.
    bool beginWebSocket(AsyncWebServer* server, const String& hostName,
                        const char* pagePath = "/debug");
    void flush() override;
    void setWebDebugLevel(uint8_t level) { if (level <= ANY) _webLevel = level; }
    uint8_t getWebDebugLevel() const { return _webLevel; }
    void setWebSilenced(bool enabled) { _webSilenced = enabled; if (enabled) clearWebBuffers(); }
    bool isWebSilenced() const { return _webSilenced; }
    size_t webClientCount() const { return _webSocket ? _webSocket->count() : 0; }
    bool hasWebClients() const { return webClientCount() > 0; }
    size_t webBatchLength() const { return _webBatch.length(); }
    size_t webBatchCapacity() const { return WEB_MAX_BATCH_LENGTH; }
    uint32_t droppedWebLines() const { return _webDroppedLines; }
    uint32_t droppedWebBytes() const { return _webDroppedBytes; }
    static int parseLevel(String level) {
        level.trim(); level.toLowerCase();
        if (level == "profiler") return PROFILER;
        if (level == "verbose" || level == "v") return VERBOSE;
        if (level == "debug" || level == "d") return DEBUG;
        if (level == "info" || level == "i") return INFO;
        if (level == "warning" || level == "warn" || level == "w") return WARNING;
        if (level == "error" || level == "e") return ERROR;
        if (level == "any" || level == "a") return ANY;
        return -1;
    }
    static const char* levelName(uint8_t level) {
        switch (level) {
            case PROFILER: return "profiler";
            case VERBOSE: return "verbose";
            case DEBUG: return "debug";
            case INFO: return "info";
            case WARNING: return "warning";
            case ERROR: return "error";
            case ANY: return "any";
            default: return "unknown";
        }
    }
    void sendWebMessage(const String& message) {
        if (_webSocket && hasWebClients() && !_webSilenced) _webSocket->textAll(message);
    }
    void sendWebMessage(AsyncWebSocketClient* client, const String& message) {
        if (client) client->text(message);
    }
    void showWebDebugLevel(bool enabled) { _webShowLevel = enabled; }
    void showWebProfiler(bool enabled) { _webShowProfiler = enabled; if (enabled) _webLastLogTime = millis(); }
#endif

	void silence(boolean activate, boolean showMessage = true);
	boolean isSilence();

	void onConnection(boolean connected);

	boolean isConnected();

#ifdef DEBUGGER_ENABLED
	// For Simple software debugger - based on SerialDebug Library
	void initDebugger(boolean (*callbackEnabled)(), void (*callbackHandle)(const boolean), String (*callbackGetHelp)(), void (*callbackProcessCmd)());
	WiFiClient* getTelnetClient();
#endif

	// Print

	virtual size_t write(uint8_t);

    virtual size_t write(const uint8_t *buffer, size_t size); // Write byte buffer

    // Debug levels

	static const uint8_t PROFILER = 0; 	// Used for show time of execution of pieces of code(profiler)
	static const uint8_t VERBOSE = 1; 	// Used for show verboses messages
	static const uint8_t DEBUG = 2;   	// Used for show debug messages
	static const uint8_t INFO = 3;		// Used for show info messages
	static const uint8_t WARNING = 4;	// Used for show warning messages
	static const uint8_t ERROR = 5;		// Used for show error messages
	static const uint8_t ANY = 6;		// Used for show always messages, for any current debug level

	// Expand characters as CR/LF to \\r, \\n

	String expand(const String& string);

	// Destructor

	~RemoteDebug();
	RemoteDebug(const RemoteDebug&) = delete;
	RemoteDebug& operator=(const RemoteDebug&) = delete;

private:

	// Variables

	String _hostName = "";				// Host name

	boolean _connected = false;			// Client is connected ?

	String _password = "";				// Password

	boolean _passwordOk = false; 		// Password request ? - 18/07/18
	uint8_t _passwordAttempt = 0;

	boolean _silence = false;			// Silence mode ?
	
	uint8_t _clientDebugLevel = DEBUG;	// Level setted by user in web app or telnet client
	uint8_t _lastDebugLevel = DEBUG;	// Last Level setted by active()

	uint32_t _lastTimePrint = millis(); // Last time print a line

	uint8_t _levelBeforeProfiler=DEBUG;	// Last Level before Profiler level

	boolean _showTime = false;			// Show time in millis

	boolean _showProfiler = false;		// Show time between messages
	uint32_t _minTimeShowProfiler = 0;	// Minimal time to show profiler

	boolean _showDebugLevel = true;		// Show debug Level

	boolean _showColors = false;		// Show colors

	boolean _showRaw = false;			// Show in raw mode ?

	boolean _serialEnabled = false;		// Send to serial too (not recommended)
	bool _telnetRoutePending = false;	// A logical message has captured Telnet/serial routing
	bool _telnetRouteActive = false;	// Current logical message is enabled for Telnet/serial

	boolean _resetCommandEnabled=false;	// Enable command to reset the board

	boolean _newLine = true;			// New line write ?

	String _command = "";				// Command received
	bool _commandOverflow = false;		// Overlong command was discarded
	uint8_t _telnetNegotiationBytesRemaining = 0;
	String _lastCommand = "";			// Last Command received
	uint32_t _lastTimeCommand = millis();// Last time command received
	String _helpProjectCmds = "";		// Help of comands setted by project (sketch)
	void (*_callbackProjectCmds)() = NULL; // Callable for projects commands
	void (*_callbackNewClient)() = NULL; // Callable for when have a new client connected

	String _filter = "";				// Filter
	boolean _filterActive = false;

#if REMOTEDEBUG_ENABLE_WEBSOCKET
	static constexpr size_t WEB_MAX_LINE_LENGTH = REMOTEDEBUG_WEB_MAX_LINE_LENGTH;
	static constexpr size_t WEB_MAX_BATCH_LENGTH = REMOTEDEBUG_WEB_MAX_BATCH_LENGTH;
    static constexpr uint32_t WEB_BATCH_INTERVAL_MS = 25;
    AsyncWebSocket* _webSocket = nullptr;
    uint8_t _webLevel = DEBUG;
    bool _webSilenced = false;
    bool _webRoutePending = false;
    bool _webRouteActive = false;
    uint8_t _webRouteLevel = ANY;
    uint8_t _webLineLevel = ANY;
    bool _webLineHasLevel = false;
    String _webLine;
    String _webBatch;
    uint32_t _webLastBatchSend = 0;
    uint32_t _webLastCleanup = 0;
    uint32_t _webDroppedLines = 0;
    uint32_t _webDroppedBytes = 0;
    uint32_t _webLastLogTime = 0;
    bool _webShowLevel = true;
    bool _webShowTime = false;
    bool _webShowProfiler = true;
    String _webFilter;
    bool _webFilterActive = false;
    void writeWebCharacter(uint8_t character);
    void flushWebSocketLine();
    void sendWebBatch(bool force = false);
    void clearWebBuffers() { _webLine = ""; _webBatch = ""; _webDroppedLines = 0; _webDroppedBytes = 0; }
    String makeWebPrefix(uint8_t level);
    void handleWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                              AwsEventType type, void* arg, uint8_t* data, size_t len);
    void processWebCommand(AsyncWebSocketClient* client, String command);
    String makeWebHelp() const;
    void sendWebWelcome(AsyncWebSocketClient* client);
#endif

	WiFiServer* _telnetServer = nullptr;
	WiFiClient _telnetClient;
	bool (*_clientAuthorizer)(const IPAddress&) = nullptr;
	uint32_t _nextConnectionAllowed = 0;
	uint8_t _consecutiveAuthFailures = 0;

	String _bufferPrint = "";			// Buffer of print write to WiFi

#ifdef CLIENT_BUFFERING
	String _bufferSend = "";			// Buffer to send data to web app or telnet client
	size_t _sizeBufferSend = 0;		// Size of it
	uint32_t _lastTimeSend = 0;			// Last time command send data
#endif

#ifdef DEBUGGER_ENABLED
	uint32_t _debuggerNextHandle = 0;
	boolean _debuggerLastConnected = false;
//	// For Simple software debugger - based on SerialDebug Library
	boolean (*_callbackDbgEnabled)() = NULL;// Callable for debugger enabled
	void (*_callbackDbgHandle)(const boolean) = NULL;	// Callable for handle of debugger
	String (*_callbackDbgHelp)() = NULL;	// Callable for get debugger help
	void (*_callbackDbgProcessCmd)() = NULL;// Callable for process commands of debugger
#endif

	//////// Privates

	void showHelp();
	void processCommand();
	void sendTelnetCommand(uint8_t command, uint8_t option);
	void setTelnetPasswordEcho(bool passwordEntry);
	String formatNumber(uint32_t value, uint8_t size, char insert='0');
	boolean isCRLF(char character);
	uint32_t getFreeMemory();


};

#else // DEBUG_DISABLED

// Disable debug macros

#define rdebugAln(...)
#define rdebugPln(...)
#define rdebugVln(...)
#define rdebugDln(...)
#define rdebugIln(...)
#define rdebugWln(...)
#define rdebugEln(...)
#define rdebug(...)

#define DEBUG(...)

#define DEBUG_A(...)
#define DEBUG_P(...)
#define DEBUG_V(...)
#define DEBUG_D(...)
#define DEBUG_I(...)
#define DEBUG_W(...)
#define DEBUG_E(...)

#define debugA(...)
#define debugP(...)
#define debugV(...)
#define debugD(...)
#define debugI(...)
#define debugW(...)
#define debugE(...)

#define rprintA(...)
#define rprintV(...)
#define rprintD(...)
#define rprintI(...)
#define rprintW(...)
#define rprintE(...)

#define rprintAln(...)
#define rprintVln(...)
#define rprintDln(...)
#define rprintIln(...)
#define rprintWln(...)
#define rprintEln(...)

#define debugHandle()

// Note all of Debug. codes need uses "#ifndef DEBUG_DISABLED"
// For example, the initialization codes and:

//#ifndef DEBUG_DISABLED
//if (Debug.isActive(Debug.VERBOSE)) {
//    Debug.printf("bla bla bla: %d %s", number, str); // OR
//    Debug.printf("bla bla bla: %d %s", number, str.c_str()); // Note: if type is String need c_str() // OR
//    Debug.println("bla bla bla 2 ln");
//}
//#endif

#endif // DEBUG_DISABLED

#endif // H

//////// End
