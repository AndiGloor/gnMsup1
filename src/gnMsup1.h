/*  GN Master Slave Universal Protocol Library
 *  ==========================================
 *  
 *  Library for generic Master/Slave Communications.
 *  
 *  Tested with Arduino UNOor MEGA2650 and RS485 BUS.
 *  
 *  2019-07-30  V1.2.2		Andreas Gloor            Bugfix begin (correct Datatype)
 *  2018-09-12  V1.2.1		Andreas Gloor            Bugfix pollRange (for FullyAsynchonous) and pushBlockingActive public; handleCommunication called if data available before send
 *  2018-08-24  V1.1.1		Andreas Gloor            SourceAddress Parameter in Callback Function
 *  2018-07-21  V1.0.1		Andreas Gloor            Initial Version
 *  
 *	MIT License
 *	
 *	Copyright (c) 2018 Andreas Gloor
 *	
 *	Permission is hereby granted, free of charge, to any person obtaining a copy
 *	of this software and associated documentation files (the "Software"), to deal
 *	in the Software without restriction, including without limitation the rights
 *	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *	copies of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions:
 *	
 *	The above copyright notice and this permission notice shall be included in all
 *	copies or substantial portions of the Software.
 *	
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *	SOFTWARE.
 */

 
 
 // Assure the Library gets loaded not more than once
#ifndef gnMsup1_h
#define gnMsup1_h



// Include Config File -> Please just change Values in Config File, not here
#include "config.h"



// Internal defines
#define GNMSUP1_NOTINSTORE									0xFF
#define GNMSUP1_MASTERPSEUDOADDRESS					0xFF
#define GNMSUP1_SYSTEMSERVICENUMBER					0xFF
#define GNMSUP1_SYSTEMSERVICE_QUERYALIVE		0x00
#define GNMSUP1_SYSTEMSERVICE_IGNORE				0x01
#define GNMSUP1_FRAMEFLAG_DIRECTION					7
#define GNMSUP1_FRAMEFLAG_SERVICE						6
#define GNMSUP1_FRAMEFLAG_PUSH							5
#define GNMSUP1_FRAMEFLAG_COMMITRECEIVE			4
#define GNMSUP1_FRAMESTART1									0xAA
#define GNMSUP1_FRAMESTART2									0x55
#define GNMSUP1_FRAMESTOP1									0xCC
#define GNMSUP1_FRAMESTOP2									0x33
#define GNMSUP1_FRAMEBUF_FLAG								0
#define GNMSUP1_FRAMEBUF_ADDRESS						1
#define GNMSUP1_FRAMEBUF_PAYLOADSIZE				2
#define GNMSUP1_FRAMEBUF_SERVICE						3
#define GNMSUP1_FRAMEBUF_SUBSERVICE					4
#define GNMSUP1_FRAMEBUF_PAYLOADSTART				5
#define GNMSUP1_PUSHSTOREEMPTY							0xFF
#define GNMSUP1_PUSHSTOREFULL								0xFF
#define GNMSUP1_PUSHBUFFLAG_PENDING					7
#define GNMSUP1_PUSHBUFFLAG_COMMITRECEIVE		6
#define GNMSUP1_RS485_DEENABLEWAITMICROS		0



// Include necessary Library's
#include <Arduino.h>
#include <HardwareSerial.h>
#include <SoftwareSerial.h>
#include <Stream.h>



// Main Class
class gnMsup1 {
	public:
		enum HardwareLayer	{RS485};
		enum Role						{Slave, Master};
		enum BlockingMode		{Synchronous, NearlyAsynchronous, FullyAsynchronous};
		enum ComErrorCode		{None, Err_CRTimeout, Err_CRInvalid};
				
		// Constructor - Overloaded with Hardware- or SoftwareSerial.
		gnMsup1(HardwareSerial& device, gnMsup1::HardwareLayer hwLayer, uint8_t rs485DePin, gnMsup1::Role role);
		gnMsup1(SoftwareSerial& device, gnMsup1::HardwareLayer hwLayer, uint8_t rs485DePin, gnMsup1::Role role);
		
		// Begin (for Serial, with Default-Baudrate)
		bool begin() {return begin(_baudrate);}
		bool begin(int32_t baudRate, uint8_t address = GNMSUP1_MASTERPSEUDOADDRESS);
		
		// HandleCommunication - Call this Function during the loop in your Sketch; avoid long delays; Data-Processing happens in callback functions.
		void handleCommunication();
		
		// AttachService - Attaches a Callback Function for a Service (identified by ServiceNumber)
		typedef void (*ServiceHandlerCallback) (uint8_t subserviceNumber, uint8_t payload[], uint8_t payloadSize, uint8_t sourceAddress);
		typedef void (*CatchAllServiceHandlerCallback) (uint8_t serviceNumber, uint8_t subserviceNumber, uint8_t payload[], uint8_t payloadSize, uint8_t sourceAddress);
		bool attachService(uint8_t serviceNumber, ServiceHandlerCallback serviceHandler);
		bool attachCatchAllService(CatchAllServiceHandlerCallback serviceHandler) {
			_callbackCatchAllActive = true;
			_callbackCatchAllHandler = serviceHandler;
		};
		
		// Push - Sends a Frame back to Master; keep in mind to poll the messages in your master code
		bool push(uint8_t serviceNumber, uint8_t subserviceNumber, bool commitReceivedFlag = false) {
			uint8_t empty[0];
			return push(serviceNumber, subserviceNumber, empty, 0, commitReceivedFlag);
		}
		bool push(uint8_t serviceNumber, uint8_t subserviceNumber, uint8_t payload[], uint8_t payloadSize, bool commitReceivedFlag = false);
		bool pushBlockingActive() {return millis() - _pushBlockingTimer < GNMSUP1_PUSHTIMEOUT;};
		// Send - Sends a Frame to a Slave
		bool send(uint8_t address, uint8_t serviceNumber, uint8_t subserviceNumber, uint8_t payload[], uint8_t payloadSize, bool pushFlag = false, bool commitReceivedFlag = false, bool retryOnCrFailure = false);
		// Poll - Sends Push-Request to a Slave or a Range of Slaves
		bool poll(uint8_t address, uint8_t maxMessagesPerSlave = 1, bool commitReceivedFlag = false, bool retryOnCrFailure = false) {
			return pollRange(address, address, maxMessagesPerSlave, commitReceivedFlag, retryOnCrFailure);
		};
		bool pollRange(uint8_t beginAddress, uint8_t endAddress, uint8_t maxMessagesPerSlave = 1, bool commitReceivedFlag = false, bool retryOnCrFailure = false);
		
		// BlockingMode
		void blockingMode(gnMsup1::BlockingMode mode) {_blockingMode = mode;};
		
		// IgnoreInactiveNodes
		bool setIgnoreInactiveNodes(bool value) {_ignoreInactiveNodes = value;};
		bool getIgnoreInactiveNodes() {return _ignoreInactiveNodes;};
				
		// GetLastComError - Provides additional Information about the Error occurred
		struct						comError_t {
												ComErrorCode						comErrorCode	= None;
												uint8_t									address				= GNMSUP1_MASTERPSEUDOADDRESS;
											};
		comError_t getLastComError();
		
		// AttachSerialDebug - Allows to output some Debug-Informations to a Serial Port (SoftwareSerial or HardwareSerial); only works if GNMSUP1_DEBUG defined
		void attachSerialDebug(HardwareSerial& device);
		void attachSerialDebug(SoftwareSerial& device);
		
		
		
	private:
		// Generic
		bool							_initialized = false;
		uint8_t						_address = 0;
		gnMsup1::BlockingMode _blockingMode = GNMSUP1_DEFAULTBLOCKING;
		bool _ownsMasterRole() {return _address == GNMSUP1_MASTERPSEUDOADDRESS;};
		
		// Serial Hardware-Layer
		HardwareLayer			_hwLayer;
		HardwareSerial*		_hwStream;
    SoftwareSerial*		_swStream;
    Stream*						_stream;
		uint32_t					_baudrate = GNMSUP1_DEFAULTBAUDRATE;						// Initialize with Default-Baudrate
		
		// RS485 Hardware-Layer
		uint8_t						_rs485DePin;
		
		// CommError
		comError_t				_lastComError;
		
		// Store for Service-Callbacks (attachService)
		struct						_callbackStore_t {
												uint8_t									serviceNumber;
												ServiceHandlerCallback	serviceHandler;
											};
		_callbackStore_t	_callbackStore[GNMSUP1_MAXSERVICECOUNT];
		uint8_t						_callbackStoreNextFree = 0;
		uint8_t _getCallbackStoreNr(uint8_t serviceNumber);
		CatchAllServiceHandlerCallback	_callbackCatchAllHandler;
		bool							_callbackCatchAllActive = false;
		
		// Frame Handling
		#if GNMSUP1_MAXPAYLOADBUFFER < (256 - 10)													// Adjust Type of BufferCounter to Buffer-Size (and Header).
			uint8_t						_framePosition = 0;
		#else
			uint16_t					_framePosition = 0;
		#endif
		uint8_t						_frameBuffer[GNMSUP1_FRAMEBUF_PAYLOADSTART + GNMSUP1_MAXPAYLOADBUFFER];
		uint16_t					_frameChecksum;
		uint32_t					_frameStartTime;
		uint16_t					_frameTimeout;
		bool 							_additionalPushMsgAvailable = false;
		bool _readInput();
		void _processFrame();
		void _resetAndStartFrame(uint8_t inputBuffer);
		void _resetFramebuffer();
		bool _validateFrame();
		bool _handleSystemService(bool pushFlag);
		
		// Store for Push-Requests
		struct						_pushStore_t {
												uint8_t									flags;
												uint32_t								timestamp;
												uint8_t									serviceNumber;
												uint8_t									subserviceNumber;
												uint8_t									payloadSize;
												uint8_t									payload[GNMSUP1_MAXPAYLOADBUFFER];
											};
		_pushStore_t			_pushStore[GNMSUP1_MAXPUSHQEUEENTRYS];
		uint32_t 					_pushBlockingTimer = millis() - GNMSUP1_PUSHTIMEOUT - 1;
		void _pushBlockingSet() {_pushBlockingTimer = millis();};
		void _pushBlockingRelease() {_pushBlockingTimer = millis() - GNMSUP1_PUSHTIMEOUT - 1;};
		void _pushBlockingWaitForRelease();
		uint8_t	_pushStoreNextToSend();
		uint8_t	_pushStoreNextFree();
		
		// SendFrame -> Internal Send Function
		bool _sendFrame(uint8_t address, uint8_t serviceNumber, uint8_t subserviceNumber, bool serviceFlag, bool pushFlag, bool waitForPushAnswer, bool commitReceivedFlag, bool retryOnCrFailure, uint8_t payload[], uint8_t payloadSize);
				
		// Active/Ignore Functions
		bool							_ignoreInactiveNodes = false;
		uint8_t 					_ignoreStore[(GNMSUP1_MAXSLAVEADDRESS + 1)];	// Set bit for Slaves with no activity; They will be ignored.
		uint8_t 					_activeStore[(GNMSUP1_MAXSLAVEADDRESS + 1)];	// Set bit for active Slaves; They wont be rescanned
		uint32_t					_scavengingInactiveLastTimestamp = 0;
		uint8_t						_scavengingInactiveNextAddress = 0;
		bool _queryIgnore(uint8_t address);
		bool _queryActive(uint8_t address);
		void _markActive(uint8_t address);
		void _markIgnore(uint8_t address);
		void _resetActiveIgnore(uint8_t address);
		void _scavengingInactive();
		
		// Debugging
		#ifdef GNMSUP1_DEBUG
			HardwareSerial*		_hwDebugStream;
			SoftwareSerial*		_swDebugStream;
			Stream*						_debugStream;
			bool							_debugAttached = false;
			void _debugPrintHex(uint8_t value);
		#endif
};
#endif	// #ifndef gnMsup1_h