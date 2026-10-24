///// Debug disable for compile to production/release ?
///// as nothing of RemotedDebug is compiled, zero overhead :-)

#ifndef DEBUG_DISABLED

///// Defines

static const char VERSION[] = "4.0.0";

///// Includes

#include "stdint.h"
#include "Arduino.h"
#include "Print.h"

#ifdef SERIAL_DEBUG_H
// Cannot used with SerialDebug at same time
#error "RemoteDebug cannot be used with SerialDebug"
#endif

// This library targets ESP32 only.
#if !defined(ESP32)
#error "RemoteDebug supports ESP32 only"
#endif
#include <WiFi.h>

#include "RemoteDebug.h" // This library


// Internal print macros for send messages to client

#define debugPrintf(fmt, ...)                        \
	{                                                \
		if (_connected)                              \
			_telnetClient.printf(fmt, ##__VA_ARGS__); \
	}
#define debugPrintln(str)              \
	{                                  \
		if (_connected)                \
			_telnetClient.println(str); \
	}
#define debugPrint(str)              \
	{                                \
		if (_connected)              \
			_telnetClient.print(str); \
	}

namespace
{
    constexpr uint8_t TELNET_IAC = 255;
    constexpr uint8_t TELNET_WILL = 251;
    constexpr uint8_t TELNET_WONT = 252;
    constexpr uint8_t TELNET_ECHO = 1;
}

// Internal debug macro - recommended stay disable

#define D(fmt, ...) // Without this
// #define D(fmt, ...) Serial.printf("rd: " fmt "\n", ##__VA_ARGS__) 	// Serial debug

////// Methods / routines

// Constructor
RemoteDebug::RemoteDebug() = default;

// Initialize the telnet server

bool RemoteDebug::begin(const String& hostName, uint8_t startingDebugLevel)
{
	return begin(hostName, TELNET_PORT, startingDebugLevel);
}

bool RemoteDebug::begin(const String& hostName, uint16_t port, uint8_t startingDebugLevel)
{
	if (startingDebugLevel > ANY || port == 0) return false;

	// Construct the server here so the caller-selected port is honoured.
	if (_telnetServer)
	{
		if (_telnetServer) _telnetServer->stop();
		delete _telnetServer;
	}
	_telnetServer = new WiFiServer(port);
	if (!_telnetServer) return false;
	_telnetServer->begin();
	_telnetServer->setNoDelay(true);

	// Reserve space to buffer of print writes

	_bufferPrint.reserve(BUFFER_PRINT);

#ifdef CLIENT_BUFFERING
	// Reserve space to buffer of send

	_bufferSend.reserve(MAX_SIZE_SEND);

#endif

	// Host name of this device

	_hostName = hostName;

	// Debug level

	_clientDebugLevel = startingDebugLevel;
	_lastDebugLevel = startingDebugLevel;

	return true;
}

#ifdef DEBUGGER_ENABLED
// Simple software debugger - based on SerialDebug Library
void RemoteDebug::initDebugger(boolean (*callbackEnabled)(), void (*callbackHandle)(const boolean), String (*callbackGetHelp)(), void (*callbackProcessCmd)())
{

	// Init callbacks for the debugger

	_callbackDbgEnabled = callbackEnabled;
	_callbackDbgHandle = callbackHandle;
	_callbackDbgHelp = callbackGetHelp;
	_callbackDbgProcessCmd = callbackProcessCmd;
}

WiFiClient *RemoteDebug::getTelnetClient()
{

	return &_telnetClient;
}

#endif

// Set the password for telnet - thanks @jeroenst for suggest thist method

void RemoteDebug::setPassword(const String& password)
{

	_password = password;
}

void RemoteDebug::sendTelnetCommand(uint8_t command, uint8_t option)
{
#if REMOTEDEBUG_HIDE_PASSWORD
	if (!_telnetClient || !_telnetClient.connected()) return;

	const uint8_t negotiation[] = { TELNET_IAC, command, option };
	_telnetClient.write(negotiation, sizeof(negotiation));
#else
	(void)command;
	(void)option;
#endif
}

void RemoteDebug::setTelnetPasswordEcho(bool passwordEntry)
{
#if REMOTEDEBUG_HIDE_PASSWORD
	// WILL ECHO tells a compliant client that the server owns echoing, which
	// disables the client's local echo. This library deliberately echoes no
	// password characters. WONT ECHO restores normal client-side echo.
	sendTelnetCommand(passwordEntry ? TELNET_WILL : TELNET_WONT, TELNET_ECHO);
#else
	(void)passwordEntry;
#endif
}

// Destructor

RemoteDebug::~RemoteDebug()
{

	// Flush

	if (_telnetClient && _telnetClient.connected())
	{
		_telnetClient.flush();
	}

	// Stop

	stop();
	delete _telnetServer;
	_telnetServer = nullptr;
}

// Stop the server

void RemoteDebug::stop()
{

	// Stop Client

	if (_telnetClient && _telnetClient.connected())
	{
		_telnetClient.stop();
	}

	// Stop server

	if (_telnetServer) _telnetServer->stop();
#ifdef REMOTEDEBUG_ENABLE_WEBSOCKET
    if (_webSocket) _webSocket->closeAll();
    clearWebBuffers();
#endif
}

// Handle the connection (in begin of loop in sketch)
// TODO: optimize when loop not have a large delay

void RemoteDebug::handle()
{
#ifdef REMOTEDEBUG_ENABLE_WEBSOCKET
    sendWebBatch();
    if (_webSocket && (uint32_t)(millis() - _webLastCleanup) >= 1000) {
        _webLastCleanup = millis();
        _webSocket->cleanupClients();
    }
#endif


#ifdef DEBUGGER_ENABLED
	if (_debuggerNextHandle == 0) _debuggerNextHandle = millis();
#endif


	// look for Client connect trial

	if (_telnetServer && _telnetServer->hasClient())
	{

		if (_telnetClient && _telnetClient.connected())
		{

			// Verify if the IP is same than actual conection

			WiFiClient newClient; // @suppress("Abstract class cannot be instantiated")
			newClient = _telnetServer->accept();
			String ip = newClient.remoteIP().toString();

			if (ip == _telnetClient.remoteIP().toString())
			{

				// Reconnect

				_telnetClient.stop();
				_telnetClient = newClient;
			}
			else
			{

				// Disconnect (not allow more than one connection)

				newClient.stop();

				return;
			}
		}
		else
		{

			// New TCP client

			_telnetClient = _telnetServer->accept();
			const uint32_t now = millis();
			if ((int32_t)(now - _nextConnectionAllowed) < 0 ||
			    (_clientAuthorizer && !_clientAuthorizer(_telnetClient.remoteIP())))
			{
				_telnetClient.stop();
				return;
			}

			// Password request ? - 18/07/18

			if (_password != "")
			{

			}
		}

		if (!_telnetClient)
		{ // No client yet ???
			return;
		}

		// Set client

		_telnetClient.setNoDelay(true); // More faster
		_telnetClient.flush();		   // clear input buffer, else you get strange characters

		// Empty buffer

		delay(100);

		while (_telnetClient.available())
		{
			_telnetClient.read();
		}

		// Connection event

		onConnection(true);
	}

	// Is client connected ? (to reduce overhead in active)

	_connected = (_telnetClient && _telnetClient.connected());

	// Get command over telnet

	if (_connected)
	{

		char last = ' '; // To avoid process two times the "\r\n"

		while (_telnetClient.available())
		{ // get data from Client

			// Get character

			const uint8_t input = static_cast<uint8_t>(_telnetClient.read());

			// Ignore the command and option bytes in Telnet negotiation replies.
			if (_telnetNegotiationBytesRemaining > 0)
			{
				--_telnetNegotiationBytesRemaining;
				continue;
			}
			if (input == TELNET_IAC)
			{
				_telnetNegotiationBytesRemaining = 2;
				continue;
			}

			const char character = static_cast<char>(input);

			// Newline (CR or LF) - once one time if (\r\n) - 26/07/17

			if (isCRLF(character) == true)
			{

				if (isCRLF(last) == false)
				{

					// Process the command

					if (_commandOverflow)
					{
						debugPrintln("* Command too long; discarded");
						_commandOverflow = false;
					}
					else if (_command.length() > 0)
					{
						_lastCommand = _command; // Store the last command
						processCommand();
					}
				}

				_command = ""; // Init it for next command
				_commandOverflow = false;
			}
			else if (isPrintable(character))
			{

				// Bound memory usage from untrusted Telnet input.
#ifdef REMOTEDEBUG_MAX_COMMAND_LENGTH
				if (_command.length() < REMOTEDEBUG_MAX_COMMAND_LENGTH)
					_command.concat(character);
				else
					_commandOverflow = true;
#else
				_command.concat(character);
#endif
			}

			// Last char

			last = character;
		}
	}

	// Client connected ?

	boolean connected = _connected;
	if (connected)
	{

#ifdef CLIENT_BUFFERING
		// Client buffering - send data in intervals to avoid delays or if its is too big

		if ((millis() - _lastTimeSend) >= DELAY_TO_SEND || _sizeBufferSend >= MAX_SIZE_SEND)
		{
			debugPrint(_bufferSend);
			_bufferSend = "";
			_sizeBufferSend = 0;
			_lastTimeSend = millis();
		}
#endif

#ifdef MAX_TIME_INACTIVE
#if MAX_TIME_INACTIVE > 0

		// Inactivity - close connection if not received commands from user in telnet
		// For reduce overheads

		uint32_t maxTime = MAX_TIME_INACTIVE; // Normal

		if (_password != "" && !_passwordOk)
		{					 // Request password - 18/08/08
			maxTime = 60000; // One minute to password
		}

		if ((millis() - _lastTimeCommand) > maxTime)
		{

			debugPrintln("* Closing session by inactivity");

			// Disconnect

			disconnect();
			return;
		}
#endif
#endif
	}

#ifdef DEBUGGER_ENABLED

	// For Simple software debugger - based on SerialDebug Library

	// Changed handle debugger logic - 2018-03-01

	if (_callbackDbgEnabled && _callbackDbgHandle)
	{ // Calbacks ok ?

		boolean callHandle = false;

		if (_debuggerLastConnected != connected)
		{ // Change connection -> always call

			_debuggerLastConnected = connected;
			callHandle = true;
		}
		else if (millis() >= _debuggerNextHandle)
		{

			if (_callbackDbgEnabled())
			{ // Only if it is enabled
				callHandle = true;
			}
		}

		if (callHandle)
		{

			// Call the handle

			_callbackDbgHandle(true);

			// Save time

			_debuggerNextHandle = millis() + DEBUGGER_HANDLE_TIME;
		}
	}
#endif

	// DV("*handle time: ", (millis() - timeBegin));
}

// Disconnect client

void RemoteDebug::disconnect(boolean only_telnetClient)
{

	// Disconnect

	if (_connected) setTelnetPasswordEcho(false);

	if (only_telnetClient)
	{
		if (_connected)
		{
			_telnetClient.println("* Closing client connection ..."); // this is to web app new conn not receive it
		}
	}
	else
	{
		debugPrintln("* Closing client connection ...");
	}

	_silence = false;

	if (_connected)
	{ // By telnet
		_telnetClient.stop();
		_connected = false;
	}
#ifdef REMOTEDEBUG_ENABLE_WEBSOCKET
    if (!only_telnetClient && _webSocket) {
        _webSocket->closeAll();
        clearWebBuffers();
    }
#endif
}

// Connection/disconnection event

void RemoteDebug::onConnection(boolean connected)
{

	// Clear variables

	D("rd onconn %d", connected);

	_bufferPrint = ""; // Clean buffer

	_lastTimeCommand = millis(); // To mark time for inactivity

	_command = "";	   // Clear command
	_lastCommand = ""; // Clear las command

	_lastTimePrint = millis(); // Clear the time

	_silence = false; // No silence

#ifdef CLIENT_BUFFERING
	// Client buffering - send data in intervals to avoid delays or if its is too big
	_bufferSend = "";
	_sizeBufferSend = 0;
	_lastTimeSend = millis();
#endif

	// Password request ? - 18/07/18

	if (_password != "")
	{

		_passwordOk = false;

#ifdef REMOTEDEBUG_PWD_ATTEMPTS
		_passwordAttempt = 1;
#endif
	}

	// Save it

	_connected = connected;

	// Process

	if (connected)
	{ // Connected ?

		// Callback

		if (_callbackNewClient)
		{
			_callbackNewClient();
		}

		// Show the initial message

#if SHOW_HELP
		showHelp();
#endif
	}
}

boolean RemoteDebug::isConnected()
{

	// Connected through either transport
#ifdef REMOTEDEBUG_ENABLE_WEBSOCKET
    return _connected || hasWebClients();
#else
    return _connected;
#endif
}

// Send to serial too (use only if need)

void RemoteDebug::setSerialEnabled(boolean enable)
{

	_serialEnabled = enable;
}

// Allow ESP reset over telnet client

void RemoteDebug::setResetCmdEnabled(boolean enable)
{
	_resetCommandEnabled = enable;
}

// Show time in millis

void RemoteDebug::showTime(boolean show)
{
	_showTime = show;
}

// Show profiler - time in millis between messages of debug

void RemoteDebug::showProfiler(boolean show, uint32_t minTime)
{
	_showProfiler = show;
	_minTimeShowProfiler = minTime;
}


// Show debug level

void RemoteDebug::showDebugLevel(boolean show)
{
	_showDebugLevel = show;
}

// Show colors

void RemoteDebug::showColors(boolean show)
{
	_showColors = show;
}

// Show in raw mode - only data ?

void RemoteDebug::showRaw(boolean show)
{
	_showRaw = show;
}

// Is active ? client telnet connected and level of debug equal or greater then set by user in telnet

boolean RemoteDebug::isActive(uint8_t debugLevel) const
{
    const bool telnetActive = (debugLevel >= _clientDebugLevel && !_silence &&
                               (_connected || _serialEnabled));
#ifdef REMOTEDEBUG_ENABLE_WEBSOCKET
    const bool webActive = (_webSocket && hasWebClients() && 
                            !_webSilenced && !_silence && debugLevel >= _webLevel);
    return telnetActive || webActive;
#else
    return telnetActive;
#endif
}
boolean RemoteDebug::beginMessage(uint8_t debugLevel)
{
    const bool telnetActive = (debugLevel >= _clientDebugLevel && !_silence &&
                               (_connected || _serialEnabled));
    if (telnetActive) _lastDebugLevel = debugLevel;
#ifdef REMOTEDEBUG_ENABLE_WEBSOCKET
    _webRouteLevel = debugLevel;
    _webRouteActive = (_webSocket && hasWebClients() && !_webSilenced && !_silence && debugLevel >= _webLevel);
    _webRoutePending = telnetActive || _webRouteActive;
    return _webRoutePending;
#else
    return telnetActive;
#endif
}
// Set help for commands over telnet set by sketch

void RemoteDebug::setHelpProjectsCmds(const String& help)
{

	_helpProjectCmds = help;
}

// Set callback of sketch function to process project messages

void RemoteDebug::setCallBackProjectCmds(void (*callback)())
{
	_callbackProjectCmds = callback;
}

void RemoteDebug::setCallBackNewClient(void (*callback)())
{
	_callbackNewClient = callback;
}

// Print

size_t RemoteDebug::write(const uint8_t *buffer, size_t size)
{

	// Process buffer
	// Write byte buffer

	for (size_t i = 0; i < size; i++)
	{
		write((uint8_t)buffer[i]);
	}

	return size;
}

size_t RemoteDebug::write(uint8_t character)
{

	// Write logic

	uint32_t elapsed = 0;

	size_t ret = 0;

#ifdef COLOR_NEW_SYSTEM
	String colorLevel = "";
#endif

	// Connected ?

	boolean connected = _connected;
#ifdef REMOTEDEBUG_ENABLE_WEBSOCKET
    writeWebCharacter(character);
#endif

	// In silente mode now ?

	if (_silence)
	{
		return 0;
	}

	// New line writted before ?

	if (_newLine)
	{

#ifdef DEBUGGER_ENABLED

		// For Simple software debugger - based on SerialDebug Library

		// Changed handle debugger logic - 2018-02-29

		if (!_showRaw)
		{ // Not for raw mode

			if (_callbackDbgEnabled && _callbackDbgEnabled())
			{ // Callbacks ok

				if (connected && _callbackDbgEnabled())
				{ // Only call if is connected and debugger is enabled

					// Call the handle

					_callbackDbgHandle(false);
				}
			}
		}
#endif

		String show = "";

		// Not in raw mode (only data)

		if (!_showRaw)
		{

#ifdef COLOR_NEW_SYSTEM

			// New color system

			if (_showColors)
			{
				switch (_lastDebugLevel)
				{
				case VERBOSE:
					show = COLOR_VERBOSE;
					break;
				case DEBUG:
					show = COLOR_DEBUG;
					break;
				case INFO:
					show = COLOR_INFO;
					break;
				case WARNING:
					show = COLOR_WARNING;
					break;
				case ERROR:
					show = COLOR_ERROR;
					break;
				}
				colorLevel = show;
			}

			// Show debug level

			if (_showDebugLevel)
			{
				switch (_lastDebugLevel)
				{
				case PROFILER:
					show.concat("(P");
					break;
				case VERBOSE:
					show.concat("(V");
					break;
				case DEBUG:
					show.concat("(D");
					break;
				case INFO:
					show.concat("(I");
					break;
				case WARNING:
					show.concat("(W");
					break;
				case ERROR:
					show.concat("(E");
					break;
				}
			}

			// Show time in millis

			if (_showTime)
			{
				if (show != "")
				{
					show.concat(" ");
				}
				show.concat("t:");
				show.concat(millis());
				show.concat("ms");
			}

			// Show profiler (time between messages)

			if (_showProfiler)
			{
				elapsed = (millis() - _lastTimePrint);
				boolean resetColors = false;
				if (show != "")
				{
					show.concat(" ");
				}
				if (_showColors)
				{
					if (elapsed < 250)
					{
						; // not color this
					}
					else if (elapsed < 1000)
					{
						show.concat(COLOR_BLACK);
						show.concat(COLOR_BACKGROUND_GREEN);
						resetColors = true;
					}
					else if (elapsed < 3000)
					{
						show.concat(COLOR_BLACK);
						show.concat(COLOR_BACKGROUND_YELLOW);
						resetColors = true;
					}
					else if (elapsed < 5000)
					{
						show.concat(COLOR_WHITE);
						show.concat(COLOR_BACKGROUND_MAGENTA);
						resetColors = true;
					}
					else
					{
						show.concat(COLOR_WHITE);
						show.concat(COLOR_BACKGROUND_RED);
						resetColors = true;
					}
				}
				show.concat("p:^");
				show.concat(formatNumber(elapsed, 4));
				show.concat("ms");
				if (resetColors)
				{
					show.concat(COLOR_RESET);
#ifdef COLOR_NEW_SYSTEM
					show.concat(colorLevel);
#endif
				}
				_lastTimePrint = millis();
			}

#else // Old colors way

			// Show debug level

			if (_showDebugLevel)
			{
				show = "(";
				if (_showColors == false)
				{
					switch (_lastDebugLevel)
					{
					case PROFILER:
						show.concat("P");
						break;
					case VERBOSE:
						show.concat("V");
						break;
					case DEBUG:
						show.concat("D");
						break;
					case INFO:
						show.concat("I");
						break;
					case WARNING:
						show.concat("W");
						break;
					case ERROR:
						show.concat("E");
						break;
					}
				}
				else
				{
					switch (_lastDebugLevel)
					{
					case PROFILER:
						show.concat("P");
						break;
					case VERBOSE:
						show.concat("V");
						break;
					case DEBUG:
						show = COLOR_BACKGROUND_GREEN;
						show.concat("D");
						break;
					case INFO:
						show = COLOR_BACKGROUND_WHITE;
						show.concat("I");
						break;
					case WARNING:
						show = COLOR_BACKGROUND_YELLOW;
						show.concat("W");
						break;
					case ERROR:
						show = COLOR_BACKGROUND_RED;
						show.concat("E");
						break;
					}
					if (show.length() > 1)
					{
						show.concat(COLOR_RESET);
					}
				}
			}

			// Show time in millis

			if (_showTime)
			{
				if (show != "")
					show.concat(" ");
				show.concat("t:");
				show.concat(millis());
				show.concat("ms");
			}

			// Show profiler (time between messages)

			if (_showProfiler)
			{
				elapsed = (millis() - _lastTimePrint);
				boolean resetColors = false;
				if (show != "")
					show.concat(" ");
				if (_showColors)
				{
					if (elapsed < 250)
					{
						; // not color this
					}
					else if (elapsed < 1000)
					{
						show.concat(COLOR_BACKGROUND_CYAN);
						resetColors = true;
					}
					else if (elapsed < 3000)
					{
						show.concat(COLOR_BACKGROUND_YELLOW);
						resetColors = true;
					}
					else if (elapsed < 5000)
					{
						show.concat(COLOR_BACKGROUND_MAGENTA);
						resetColors = true;
					}
					else
					{
						show.concat(COLOR_BACKGROUND_RED);
						resetColors = true;
					}
				}
				show.concat("p:^");
				show.concat(formatNumber(elapsed, 4));
				show.concat("ms");
				if (resetColors)
				{
					show.concat(COLOR_RESET);
				}
				_lastTimePrint = millis();
			}

#endif
		}
		else
		{ // Raw mode - only data - e.g. used for debugger messages

#ifdef COLOR_NEW_SYSTEM
			show.concat(COLOR_RAW);
#endif
		}

		// Show anything ?

		if (show != "")
		{

			if (!_showRaw)
			{
				show.concat(") ");
			}

			// Write to telnet buffered

			if (connected || _serialEnabled)
			{ // send data to Client
				_bufferPrint = show;
			}
		}

		_newLine = false;
	}

	// Print ?

	boolean doPrint = false;

	// New line ?

	if (character == '\n')
	{

		_bufferPrint.concat("\r"); // Para clientes windows - 29/01/17

		_newLine = true;
		doPrint = true;
	}
	else if (_bufferPrint.length() + 1 >= BUFFER_PRINT)
	{ // Limit of buffer

		doPrint = true;
	}

	// Write to telnet Buffered

	_bufferPrint.concat((char)character);

	// Send the characters buffered by print.h

	if (doPrint)
	{ // Print the buffer

		boolean noPrint = false;

		if (_showProfiler && elapsed < _minTimeShowProfiler)
		{ // Profiler time Minimal
			noPrint = true;
		}
		else if (_filterActive)
		{ // Check filter before print

			String aux = _bufferPrint;
			aux.toLowerCase();

			if (aux.indexOf(_filter) == -1)
			{ // not find -> no print
				noPrint = true;
			}
		}

		if (noPrint == false)
		{

#ifdef COLOR_NEW_SYSTEM
			_bufferPrint.concat(COLOR_RESET);
#endif
			// Send to telnet or websocket (buffered)

			boolean sendToClient = connected;

			if (_password != "" && !_passwordOk)
			{ // With no password -> no telnet output - 2018-10-19
				sendToClient = false;
			}

			if (sendToClient)
			{ // send data to Client

#ifndef CLIENT_BUFFERING
				debugPrint(_bufferPrint);
#else // Cliente buffering

				const size_t size = _bufferPrint.length();

				// Buffer too big ?

				if ((_sizeBufferSend + size) >= MAX_SIZE_SEND)
				{

					// Send it

					debugPrint(_bufferSend);
					_bufferSend = "";
					_sizeBufferSend = 0;
					_lastTimeSend = millis();
				}

				// Add to buffer of send

				_bufferSend.concat(_bufferPrint);
				_sizeBufferSend += size;

				// Client buffering - send data in intervals to avoid delays or if its is too big
				// Not for raw mode
				if (_showRaw || (millis() - _lastTimeSend) >= DELAY_TO_SEND)
				{
					debugPrint(_bufferSend);
					_bufferSend = "";
					_sizeBufferSend = 0;
					_lastTimeSend = millis();
				}
#endif
			}

			// Echo to serial (not buffering it)
			if (_serialEnabled)
			{
				// remove color codes before printing to serial
				const char *color_codes[] = {
					COLOR_RESET,
					COLOR_BLACK,
					COLOR_RED,
					COLOR_GREEN,
					COLOR_YELLOW,
					COLOR_BLUE,
					COLOR_MAGENTA,
					COLOR_CYAN,
					COLOR_WHITE,
					COLOR_DARK_BLACK,
					COLOR_LIGHT_RED,
					COLOR_LIGHT_GREEN,
					COLOR_LIGHT_YELLOW,
					COLOR_LIGHT_BLUE,
					COLOR_LIGHT_MAGENTA,
					COLOR_LIGHT_CYAN,
					COLOR_LIGHT_WHITE,
					COLOR_BACKGROUND_BLACK,
					COLOR_BACKGROUND_RED,
					COLOR_BACKGROUND_GREEN,
					COLOR_BACKGROUND_YELLOW,
					COLOR_BACKGROUND_BLUE,
					COLOR_BACKGROUND_MAGENTA,
					COLOR_BACKGROUND_CYAN,
					COLOR_BACKGROUND_WHITE};

				const int num_color_codes = sizeof(color_codes) / sizeof(color_codes[0]);

				for (int i = 0; i < num_color_codes; i++)
				{
					_bufferPrint.replace(color_codes[i], "");
				}
				Serial.print(_bufferPrint);
			}
		}

		// Empty the buffer

		ret = _bufferPrint.length();
		_bufferPrint = "";
	}

	// Retorna

	return ret;
}

////// Private

// Show help of commands

void RemoteDebug::showHelp()
{

	// Show the initial message

	String help = "";

	// Password request ? - 04/03/18

	if (_password != "" && !_passwordOk)
	{

		setTelnetPasswordEcho(true);
		help.concat("\r\n");
		help.concat("* Please enter with a password to access");
#ifdef REMOTEDEBUG_PWD_ATTEMPTS
		help.concat(" (attempt ");
		help.concat(_passwordAttempt);
		help.concat(" of ");
		help.concat(REMOTEDEBUG_PWD_ATTEMPTS);
		help.concat(")");
#endif
		help.concat(':');
		help.concat("\r\n");

		debugPrint(help);

		return;
	}

	// Show help

	help.concat("*** Remote debug - over Telnet - for ESP32 - version ");
	help.concat(VERSION);
	help.concat("\r\n");
	help.concat("* Host name: ");
	help.concat(_hostName);
	help.concat(" IP:");
	help.concat(WiFi.localIP().toString());
	help.concat(" Mac address:");
	help.concat(WiFi.macAddress());
	help.concat("\r\n");
	help.concat("* Free Heap RAM: ");
	help.concat(ESP.getFreeHeap());
	help.concat("\r\n");
	help.concat("* ESP SDK version: ");
	help.concat(ESP.getSdkVersion());
	help.concat("\r\n");
	help.concat("******************************************************\r\n");
	help.concat("* Commands:\r\n");
	help.concat("    ? or help -> display these help of commands\r\n");
	help.concat("    q -> quit (close this connection)\r\n");
	help.concat("    m -> display memory available\r\n");
	help.concat("    v -> set debug level to verbose\r\n");
	help.concat("    d -> set debug level to debug\r\n");
	help.concat("    i -> set debug level to info\r\n");
	help.concat("    w -> set debug level to warning\r\n");
	help.concat("    e -> set debug level to errors\r\n");
	help.concat("    s -> set debug silence on/off\r\n");
	help.concat("    l -> show debug level\r\n");
	help.concat("    t -> show time (millis)\r\n");
	help.concat("    p -> show time between actual and last message (in millis)\r\n");
	help.concat("    c -> show colors\r\n");
	help.concat("    filter:\r\n");
	help.concat("          filter <string> -> show only debugs with this\r\n");
	help.concat("          nofilter        -> disable the filter\r\n");
	if (_resetCommandEnabled)
	{
		help.concat("    reset -> reset the ESP32\r\n");
	}

	// Callbacks

	if (_helpProjectCmds != "" && (_callbackProjectCmds))
	{
		help.concat("\r\n");
		help.concat("    * Project commands:\r\n");
		String show = "\r\n";
		show.concat(_helpProjectCmds);
		show.replace("\n", "\n    "); // ident this
		help.concat(show);
	}

#ifdef DEBUGGER_ENABLED
	// Get help for the debugger

	if (_callbackDbgHelp)
	{
		help.concat("\r\n");
		help.concat(_callbackDbgHelp());
	}
#endif
	help.concat("\r\n");

	help.concat("\r\n");
	help.concat(
		"* Please type the command and press enter to execute.(? or h for this help)\r\n");
	help.concat("***\r\n");

	// Send to client

	debugPrint(help);
}

// Get last command received

String RemoteDebug::getLastCommand()
{

	return _lastCommand;
}

// Clear the last command received

void RemoteDebug::clearLastCommand()
{
	_lastCommand = "";
}

// Process user command over telnet or web socket

void RemoteDebug::processCommand()
{

	D("cmd: %s", _command.c_str());

	// Password request ? - 18/07/18

	if (_password != "" && !_passwordOk)
	{ // Process the password - 18/08/18 - adjust in 04/09/08 and 2018-10-19

		if (_command == _password)
		{

			setTelnetPasswordEcho(false);
			debugPrintln("\r\n* Password ok, allowing access now...");

			_passwordOk = true;
			_consecutiveAuthFailures = 0;
			_nextConnectionAllowed = 0;

			showHelp();
		}
		else
		{

			debugPrintln("\r\n* Wrong password!");
			++_consecutiveAuthFailures;
			const uint8_t shift = (_consecutiveAuthFailures > 5) ? 5 : _consecutiveAuthFailures;
			_nextConnectionAllowed = millis() + (REMOTEDEBUG_AUTH_BACKOFF_MS << shift);

#ifdef REMOTEDEBUG_PWD_ATTEMPTS

			_passwordAttempt++;

			if (_passwordAttempt > REMOTEDEBUG_PWD_ATTEMPTS)
			{

				debugPrintln("\r\n* Many attempts. Closing session now.");

				// Disconnect

				disconnect();
			}
			else
			{

				showHelp();
			}

#endif
		}

		return;
	}

	// Process commands

	debugPrint("* Debug: Command received: ");
	debugPrintln(_command);

	String options = "";
	const int pos = _command.indexOf(" ");
	if (pos > 0)
	{
		options = _command.substring(pos + 1);
	}

	// Set time of last command received

	_lastTimeCommand = millis();

	// Get out of silent mode

	if (_command != "s" && _silence)
	{

		silence(false, true);
	}

	// Process the command

	if (_command == "h" || _command == "?")
	{

		// Show help

		showHelp();
	}
	else if (_command == "q")
	{

		// Quit

		debugPrintln("* Closing client connection ...");

		_telnetClient.stop();
	}
	else if (_command == "m")
	{

		uint32_t free = ESP.getFreeHeap();

		debugPrint("* Free Heap RAM: ");
		debugPrintln(free);

	}
	else if (_command == "v")
	{

		// Debug level

		_clientDebugLevel = VERBOSE;

		debugPrintln("* Debug level set to Verbose");
	}
	else if (_command == "d")
	{

		// Debug level

		_clientDebugLevel = DEBUG;

		debugPrintln("* Debug level set to Debug");
	}
	else if (_command == "i")
	{

		// Debug level

		_clientDebugLevel = INFO;

		debugPrintln("* Debug level set to Info");
	}
	else if (_command == "w")
	{

		// Debug level

		_clientDebugLevel = WARNING;

		debugPrintln("* Debug level set to Warning");
	}
	else if (_command == "e")
	{

		// Debug level

		_clientDebugLevel = ERROR;

		debugPrintln("* Debug level set to Error");
	}
	else if (_command == "l")
	{

		// Show debug level

		_showDebugLevel = !_showDebugLevel;

		debugPrintf("* Show debug level: %s\r\n",
					(_showDebugLevel) ? "On" : "Off");
	}
	else if (_command == "t")
	{

		// Show time

		_showTime = !_showTime;

		debugPrintf("* Show time: %s\r\n", (_showTime) ? "On" : "Off");
	}
	else if (_command == "s")
	{

		// Toogle silence (new) = 28/08/18

		silence(!_silence);
	}
	else if (_command == "p")
	{

		// Show profiler

		_showProfiler = !_showProfiler;
		_minTimeShowProfiler = 0;

		debugPrintf("* Show profiler: %s\r\n",
					(_showProfiler) ? "On" : "Off");
	}
	else if (_command == "c")
	{

		// Show colors

		_showColors = !_showColors;

		debugPrintf("* Show colors: %s\r\n",
					(_showColors) ? "On" : "Off");
	}
	else if (_command.startsWith("filter ") && options.length() > 0)
	{

		setFilter(options);
	}
	else if (_command == "nofilter")
	{

		setNoFilter();
	}
	else if (_command == "reset" && _resetCommandEnabled)
	{

		debugPrintln("* Reset ...");

		debugPrintln("* Closing client connection ...");

		debugPrintln("* Resetting the ESP32 ...");

		_telnetClient.stop();
		if (_telnetServer) _telnetServer->stop();

		delay(500);

		// Reset

		ESP.restart();

#ifdef DEBUGGER_ENABLED
	}
	else if (!_callbackDbgProcessCmd && _command.startsWith("dbg"))
	{

		// Show a message of debugger not is active

		debugPrintln("* RemoteDebugger not activate for this project");
		debugPrintln("* Please access it to see how activate this:");
		debugPrintln("* https://github.com/lucasnz/RemoteDebugger");

#endif
	}
	else
	{

		// Callbacks

#ifdef DEBUGGER_ENABLED
		// Process commands for the debugger

		if (_callbackDbgProcessCmd)
		{
			_callbackDbgProcessCmd();
		}
#endif

		// Project commands - set by programmer

		if (_callbackProjectCmds)
		{

			_callbackProjectCmds();
		}
	}
}

// Filter

void RemoteDebug::setFilter(const String& filter)
{

	_filter = filter;
	_filter.toLowerCase(); // TODO: option to case insensitive ?
	_filterActive = true;

	debugPrint("* Debug: Filter active: ");
	debugPrintln(_filter);
}

void RemoteDebug::setNoFilter()
{

	_filter = "";
	_filterActive = false;

	debugPrintln("* Debug: Filter disabled");
}

// Silence

void RemoteDebug::silence(boolean activate, boolean showMessage)
{

	// Set silence and timeout

	if (showMessage)
	{

		if (activate)
		{

			debugPrintln("* Debug now is in silent mode!");
			debugPrintln("* Press s again or another command to return show debugs");
		}
		else
		{

			debugPrintln("* Debug now exit from silent mode!");
		}
	}

	// Set it

	_silence = activate;
#ifdef REMOTEDEBUG_ENABLE_WEBSOCKET
    _webSilenced = activate;
    if (activate) clearWebBuffers();
#endif
}

boolean RemoteDebug::isSilence()
{

	return _silence;
}

// Format numbers

String RemoteDebug::formatNumber(uint32_t value, uint8_t size, char insert)
{

	// Putting zeroes in left

	String ret = "";

	for (uint8_t i = 1; i <= size; i++)
	{
		uint32_t max = pow(10, i);
		if (value < max)
		{
			for (uint8_t j = (size - i); j > 0; j--)
			{
				ret.concat(insert);
			}
			break;
		}
	}

	ret.concat(value);

	return ret;
}

/////// Utilities

// Get free memory

uint32_t RemoteDebug::getFreeMemory()
{

	return ESP.getFreeHeap();
}

// Is CR or LF ?

boolean RemoteDebug::isCRLF(char character)
{

	return (character == '\r' || character == '\n');
}

// Expand characters as CR/LF to \\r, \\n
// TODO: make this for another chars not printable

String RemoteDebug::expand(const String& value)
{
	String string = value;
	string.replace("\r", "\\r");
	string.replace("\n", "\\n");
	return string;
}

#else // DEBUG_DISABLED

/////// All debug is disabled, this include is to define empty debug macros

#include "RemoteDebug.h" // This library

#endif // DEBUG_DISABLED

/////// End


#if defined(REMOTEDEBUG_ENABLE_WEBSOCKET) && !defined(DEBUG_DISABLED)
void RemoteDebug::flush()
{
    flushWebSocketLine();
    sendWebBatch(true);
    if (_telnetClient) _telnetClient.flush();
}

void RemoteDebug::writeWebCharacter(uint8_t character)
{
    bool active;
    uint8_t level;
    if (_webRoutePending) {
        active = _webRouteActive;
        level = _webRouteLevel;
    } else {
        active = (_webSocket && hasWebClients() && !_webSilenced && !_silence);
        level = ANY;
    }
    if (!active) {
        if (character == '\n') { _webRoutePending = false; _webRouteActive = false; }
        return;
    }
    if (character == '\r') return;
    if (!_webLineHasLevel) { _webLineLevel = level; _webLineHasLevel = true; }
    if (character == '\n') {
        flushWebSocketLine();
        _webRoutePending = false;
        _webRouteActive = false;
        _webRouteLevel = ANY;
        return;
    }
    if (_webLine.length() >= WEB_MAX_LINE_LENGTH) flushWebSocketLine();
    if (_webLine.length() < WEB_MAX_LINE_LENGTH) _webLine += (char)character;
}

void RemoteDebug::flushWebSocketLine()
{
    if (_webLine.isEmpty()) { _webLineHasLevel = false; return; }
    String line;
    line.reserve(_webLine.length() + 32);
    line = makeWebPrefix(_webLineLevel);
    line += _webLine;
    size_t needed = line.length() + 1;
    if (_webBatch.length() + needed <= WEB_MAX_BATCH_LENGTH) {
        _webBatch += line;
        _webBatch += '\n';
    } else {
        ++_webDroppedLines;
        _webDroppedBytes += needed;
    }
    _webLine = "";
    _webLineHasLevel = false;
}

void RemoteDebug::sendWebBatch(bool force)
{
    if (!_webSocket || !hasWebClients() || _webSilenced || _silence) {
        clearWebBuffers();
        return;
    }
    uint32_t now = millis();
    if (!force && (uint32_t)(now - _webLastBatchSend) < WEB_BATCH_INTERVAL_MS) return;
    if (!_webBatch.isEmpty()) { _webSocket->textAll(_webBatch); _webBatch = ""; }
    if (_webDroppedLines) {
        String warning = "[Web debug dropped "; warning += String(_webDroppedLines);
        warning += " lines / "; warning += String(_webDroppedBytes); warning += " bytes: output buffer full]";
        _webSocket->textAll(warning);
        _webDroppedLines = 0; _webDroppedBytes = 0;
    }
    _webLastBatchSend = now;
}

String RemoteDebug::makeWebPrefix(uint8_t level)
{
    if (!_webShowLevel && !_webShowProfiler) return "";
    String prefix = "(";
    if (_webShowLevel) {
        switch (level) {
            case PROFILER: prefix += 'P'; break;
            case VERBOSE: prefix += 'V'; break;
            case DEBUG: prefix += 'D'; break;
            case INFO: prefix += 'I'; break;
            case WARNING: prefix += 'W'; break;
            case ERROR: prefix += 'E'; break;
            default: prefix += 'A'; break;
        }
    }
    if (_webShowLevel && _webShowProfiler) prefix += ' ';
    if (_webShowProfiler) {
        uint32_t now = millis();
        uint32_t elapsed = _webLastLogTime ? (uint32_t)(now - _webLastLogTime) : 0;
        _webLastLogTime = now;
        prefix += "p:^";
        if (elapsed < 1000) prefix += '0';
        if (elapsed < 100) prefix += '0';
        if (elapsed < 10) prefix += '0';
        prefix += String(elapsed); prefix += "ms";
    }
    prefix += ") ";
    return prefix;
}
#endif
