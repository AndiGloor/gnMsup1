/*  GN Master Slave Universal Protocol Library
 *  ==========================================
 *  
 *  Library for generic Master/Slave Communications.
 *  
 *  Tested with Arduino UNO or MEGA2650 and RS485 BUS.
 *  
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

 
 
// Include necessary Library's
#include "gnMsup1.h"

#include <Arduino.h>
#include <HardwareSerial.h>
#include <SoftwareSerial.h>
#include <Stream.h>
#include <FastCRC.h>																																					// FastCCR by Frank Boesing, V1.3, MIT License, https://github.com/FrankBoesing/FastCRC



// Global Objects/Variables
FastCRC16 CRC16;



// Public ////////////////////////////////////////////////////////////////////////////////////////////////////////////



// Constructor
gnMsup1::gnMsup1(HardwareSerial& device, gnMsup1::HardwareLayer hwLayer, uint8_t rs485DePin, 	// RS485, HardwareSerial
								 gnMsup1::Role role) {
	if (hwLayer != gnMsup1::RS485) {																														// Validate RS485
		return;
	}
	_hwStream = &device;																																				// Store the Values
	_hwLayer = hwLayer;
	_rs485DePin = rs485DePin;
	if (role == Master) {
		_address = GNMSUP1_MASTERPSEUDOADDRESS;
	}
}

gnMsup1::gnMsup1(SoftwareSerial& device, gnMsup1::HardwareLayer hwLayer, uint8_t rs485DePin,	// RS485, SoftwareSerial
								 gnMsup1::Role role) {
	if (hwLayer != gnMsup1::RS485) {																														// Validate RS485
		return;
	}
	_swStream = &device;																																				// Store the Values
	_hwLayer = hwLayer;
	_rs485DePin = rs485DePin;
	if (role == Master) {
		_address = GNMSUP1_MASTERPSEUDOADDRESS;
	}
}



// Begin -> Call this Function to start the MSUP
bool gnMsup1::begin(uint32_t baudRate, uint8_t address) {
	if (!((_ownsMasterRole() && address == GNMSUP1_MASTERPSEUDOADDRESS) ||											// Validate Master/Slave-Address
				(!_ownsMasterRole() && address < 0xF0))) {
		#ifdef GNMSUP1_DEBUG
			if (_debugAttached) {
				_debugStream->println(F("ERR:INVALID ADDRESS/ROLE!"));
			}
		#endif

		return false;
	}
	
	_address = address;																																					// Save the Address
	
	if (_hwStream) {																																						// Call Begin Function of corresponding SerialObject...
    _hwStream->begin(baudRate);
  }
  else {
    _swStream->begin(baudRate);
  }
	
	while (!_stream) {																																					// Safe Stream from SerialObject while it is ready
		_stream = !_hwStream? (Stream*)_swStream : _hwStream;
	}
	
	if (_hwLayer == gnMsup1::RS485) {																														// Initializing RS485 (DE Pin)
		pinMode(_rs485DePin, OUTPUT);
		digitalWrite(_rs485DePin, LOW);
	}
	
	_frameTimeout = (((10 + GNMSUP1_MAXPAYLOADBUFFER) * GNMSUP1_FRAMELENGHTTIMEOUT							// Calculate Frame-Timeout: Frame-Length * Factor (Config-File)
										* (10000000ul / baudRate)																									// Multiply by Time/Byte [Microseconds]
										+ 501) / 1000);																														// Converted to [Milliseconds] (roundup)
	
	memset(_ignoreStore, 0, sizeof(_ignoreStore));
	
	#ifdef GNMSUP1_DEBUG
		if (_debugAttached) {
			_debugStream->print(F("INIT:OK\tROLE:"));
			if (_ownsMasterRole()) {
				_debugStream->print(F("MASTER"));
			} else {
				_debugStream->print(F("SLAVE\tADDR:0x"));
				_debugPrintHex(address);
			}
			_debugStream->print(F("\tTIMEOUT:"));
			_debugStream->print(_frameTimeout);
			_debugStream->print(F("ms"));
			_debugStream->println();
		}
	#endif
			
	_initialized = true;																																				// Set Init-Flag and return true
	return true;
}



// HandleCommunication -> Call this Function during the loop in your Sketch; avoid long delays
void gnMsup1::handleCommunication() {
	if (!_initialized) {																																				// Dont proceed until initialized
		return;
	}
	
	if (_readInput()) {																																					// Read the Input; Proceed when Frame complete & valid in Buffers
		_processFrame();		
	}
	
	_scavengingInactive();																																			// Scavenging inactives (use its own Timestamp based check)
}



// AttachService -> Attaches a Callback Function for a Service (identified by ServiceNumber)
bool gnMsup1::attachService(uint8_t serviceNumber, gnMsup1::ServiceHandlerCallback serviceHandler) {
	if (serviceNumber == 0xFF) {																																// Avoid register System-Service
		#ifdef GNMSUP1_DEBUG
			if (_debugAttached) {
				_debugStream->println(F("ERR:SERVICE 0xFF RESERVED FOR SYSTEM"));
			}
		#endif
		return false;
	}
	
	if (_getCallbackStoreNr(serviceNumber) == GNMSUP1_NOTINSTORE) {															// Check if allready in Store
		_callbackStore[_callbackStoreNextFree] = {serviceNumber, serviceHandler};
		_callbackStoreNextFree++;
	
		#ifdef GNMSUP1_DEBUG
			if (_debugAttached) {
				_debugStream->print(F("SERVICE 0x"));
				_debugPrintHex(serviceNumber);
				_debugStream->println(F(" ATTACHED"));
			}
		#endif
		return true;
	} else {
		#ifdef GNMSUP1_DEBUG
			if (_debugAttached) {
				_debugStream->print(F("ERR:SERVICE 0x"));
				_debugPrintHex(serviceNumber);
				_debugStream->println(F(" ALLREADY ATTACHED"));
			}
		#endif
		return false;
	}
}



// Push -> Sends a Frame back to Master; keep in mind to poll the messages in your master code
bool gnMsup1::push(uint8_t serviceNumber, uint8_t subserviceNumber, uint8_t payload[], uint8_t payloadSize, bool commitReceivedFlag) {
	if (_ownsMasterRole() || !_initialized) {																										// Only Slave is permitted to use this Function; only if initialized
		#ifdef GNMSUP1_DEBUG
			if (_debugAttached) {
				_debugStream->println();
				_debugStream->println(F("ERR: PUSH ONLY USABLE IN SLAVE ROLE, AFTER BEGIN."));
			}
		#endif
		
		return false;
	}
	
	uint8_t storePosition = _pushStoreNextFree();																								// Check if Space in Store
	if (storePosition == GNMSUP1_PUSHSTOREFULL) {
		if (_blockingMode == gnMsup1::FullyAsynchronous) {																				// FullyAsynchonous fails if Store runs out of space
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->println();
					_debugStream->print(F("ERR: PUSH FRAME TO MASTER"));
					_debugStream->print(F(", SERVICE 0x"));
					_debugPrintHex(serviceNumber);
					_debugStream->print(F(", SUBSERVICE 0x"));
					_debugPrintHex(subserviceNumber);
					_debugStream->print(F(", PAYLOAD-SIZE 0x"));
					_debugPrintHex(payloadSize);
					_debugStream->print(F(" -> PUSH-QEUE OUT OF SPACE"));
					_debugStream->println();
				}
			#endif
			
			return false;
		} else {																																									// NearlyAsynchronous waits till a space in queue gets free (due to a call or timeout)
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->println();
					_debugStream->println(F("WRN:PUSH-QEUE OUT OF SPACE, WAIT FOR MASTER OR TIMEOUT. AVOID THIS CONDITION!"));
				}
			#endif
			
			while (storePosition == GNMSUP1_PUSHSTOREFULL) {
				handleCommunication();
				storePosition = _pushStoreNextFree();
			}
		}
	}
	
	uint8_t flagbyte = 0x00;																																		// Prepare Flag Byte
	bitWrite(flagbyte, GNMSUP1_PUSHBUFFLAG_PENDING, true);
	bitWrite(flagbyte, GNMSUP1_PUSHBUFFLAG_COMMITRECEIVE, commitReceivedFlag);
	_pushStore[storePosition].flags = flagbyte;																									// Add the Entry to the Store
	_pushStore[storePosition].timestamp = millis();
	_pushStore[storePosition].serviceNumber = serviceNumber;
	_pushStore[storePosition].subserviceNumber = subserviceNumber;
	_pushStore[storePosition].payloadSize = payloadSize;
	for (uint8_t i = 0; i < payloadSize; i++) {
		_pushStore[storePosition].payload[i] = payload[i];
	}
	
	#ifdef GNMSUP1_DEBUG
		if (_debugAttached) {
			_debugStream->println();
			_debugStream->print(F("PUSH:SERVICE 0x"));
			_debugPrintHex(serviceNumber);
			_debugStream->print(F(", SUBSERVICE 0x"));
			_debugPrintHex(subserviceNumber);
			_debugStream->print(F(", PAYLOAD-SIZE 0x"));
			_debugPrintHex(payloadSize);
		}
	#endif
	
	if (_blockingMode == gnMsup1::Synchronous) {																								// Synchronous waits until the Message was delivered to Master or timeouts
		#ifdef GNMSUP1_DEBUG
		if (_debugAttached) {
			_debugStream->println(F(" -> MODE ENFORCE BLOCKING WAIT."));
		}
		#endif
		
		while (_pushStoreNextFree() != storePosition) {																						// Wait until the free StorePosition equals the current (which means its free again)
			handleCommunication();
		}
	} else {																																										// Other Modes work with regular Qeue
		#ifdef GNMSUP1_DEBUG
			if (_debugAttached) {
				_debugStream->println(F(" -> ADDED TO PUSH-QEUE"));
			}
		#endif		
	}
	return true;
}



// Send -> Sends a Frame to a Slave
bool gnMsup1::send(uint8_t address, uint8_t serviceNumber, uint8_t subserviceNumber, uint8_t payload[], uint8_t payloadSize, bool pushFlag, bool commitReceivedFlag, bool retryOnCrFailure) {
	if (!_ownsMasterRole() || !_initialized) {																									// Only Master is permitted to use this Function; only if initialized
		#ifdef GNMSUP1_DEBUG
			if (_debugAttached) {
				_debugStream->println();
				_debugStream->println(F("ERR: PUSH ONLY USABLE IN MASTER ROLE, AFTER BEGIN."));
			}
		#endif
		
		return false;
	}
	
	if (address > GNMSUP1_MAXSLAVEADDRESS) {
		#ifdef GNMSUP1_DEBUG
			if (_debugAttached) {
				_debugStream->println(F("ERR: SLAVE ADDRESS OUT OF RANGE."));
			}
		#endif
		return false;
	}
	
	if (_pushBlockingActive()) {																																// Blocking Mode dependent behavior if a push-request is pending 
		if (_blockingMode == gnMsup1::FullyAsynchronous) {																				// FullyAsynchronous fails if another Request is active
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->println();
					_debugStream->println(F("ERR: PUSH-ANSWER OPEN; FULLYASYNCHRONOUS DOESN'T ALLOW TO CALL SEND."));
				}
			#endif
		
			return false;
		} else {																																									// The other modes wait for Answer or Timeout
			_pushBlockingWaitForRelease();
		}
	}
	
	bool waitForPushAnswer = (_blockingMode == gnMsup1::Synchronous);														// Only Synchronous Mode waits for a Answer
	
	#ifdef GNMSUP1_DEBUG
		if (_debugAttached) {
			_debugStream->println();
			_debugStream->print(F("SEND FRAME TO 0x"));
			_debugPrintHex(address);
			_debugStream->print(F(", SERVICE 0x"));
			_debugPrintHex(serviceNumber);
			_debugStream->print(F(", SUBSERVICE 0x"));
			_debugPrintHex(subserviceNumber);
			_debugStream->print(F(", PAYLOAD-SIZE 0x"));
			_debugPrintHex(payloadSize);
			if (pushFlag) {
				_debugStream->print(F(", PUSHFLAG SET"));
			}
			if (commitReceivedFlag) {
				_debugStream->print(F(", WITH CR"));
			}			
			_debugStream->println();
		}
	#endif
	
	return _sendFrame(address, serviceNumber, subserviceNumber, true, pushFlag, waitForPushAnswer, commitReceivedFlag, retryOnCrFailure, payload, payloadSize);
}



// PollRange -> Sends Push-Request to a Slave or a Range of Slaves
bool gnMsup1::pollRange(uint8_t beginAddress, uint8_t endAddress, uint8_t maxMessagesPerSlave, bool commitReceivedFlag, bool retryOnCrFailure) {
	if (!_ownsMasterRole() || !_initialized) {																									// Only Master is permitted to use this Function; only if initialized
		#ifdef GNMSUP1_DEBUG
			if (_debugAttached) {
				_debugStream->println(F("ERR: PUSH ONLY USABLE IN MASTER ROLE, AFTER BEGIN."));
			}
		#endif
		return false;
	}
	
	if (endAddress > GNMSUP1_MAXSLAVEADDRESS) {
		#ifdef GNMSUP1_DEBUG
			if (_debugAttached) {
				_debugStream->println(F("ERR: SLAVE ADDRESS OUT OF RANGE."));
			}
		#endif
		return false;
	}
	
	if (maxMessagesPerSlave < 1 || _blockingMode == gnMsup1::FullyAsynchronous) {								// Minimum 1 Message per Slave, FullyAsynchonous not allowed
		#ifdef GNMSUP1_DEBUG
			if (_debugAttached) {
				_debugStream->println(F("ERR: PUSH MINIMUM 1 MESSAGE PER SLAVE OR NOT ALLOWED IN FULLYASYNCHRONOUS MODE."));
			}
		#endif
		return false;
	}
	
	if (_blockingMode == gnMsup1::FullyAsynchronous && beginAddress != endAddress) {						// FullyAsynchronous Mode doesn't allow range-polling
		#ifdef GNMSUP1_DEBUG
			if (_debugAttached) {
				_debugStream->println(F("ERR: FULLYASYNCHRONOUS DOESN'T ALLOW RANGE POLLING."));
			}
		#endif
		
		return false;
	}
	
	if (_pushBlockingActive()) {																																// Blocking Mode dependent behavior if a push-request is pending 
		if (_blockingMode == gnMsup1::FullyAsynchronous) {																				// FullyAsynchronous fails if another Request is active
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->println();
					_debugStream->println(F("ERR: PUSH-ANSWER OPEN; FULLYASYNCHRONOUS DOESN'T ALLOW TO CALL ANOTHER POLL."));
				}
			#endif
		
			return false;
		} else {																																									// The other modes wait for Answer or Timeout
			_pushBlockingWaitForRelease();
		}
	}
	
	bool returnValue = true;
	uint8_t payload[0];
	bool waitForPushAnswer = false;
	
	for (uint8_t address = beginAddress; address <= endAddress; address++) {
		for (uint8_t remainingMessages = maxMessagesPerSlave;																			// Iterate all remainingMessages per Slave
					remainingMessages > 0; remainingMessages--) {
		
			switch (_blockingMode) {																																// Determine the Wait for Push Answer:
				case gnMsup1::FullyAsynchronous:																											// FullyAsynchronous never waits
					waitForPushAnswer = false;
					break;
				case gnMsup1::NearlyAsynchronous:																											// NearlyAsynchronous doesn't wait the last
					waitForPushAnswer = (!((address == endAddress) && (remainingMessages <= 1)));
					break;
				default:																																							// Synchronous always waits
					waitForPushAnswer = true;
			}
		
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->println();
					_debugStream->print(F("SEND PUSH-REQUEST TO 0x"));
					_debugPrintHex(address);
					_debugStream->print(F(", WAIT:"));
					_debugStream->print(waitForPushAnswer);
					_debugStream->print(F(", MAXMSG:"));
					_debugStream->print(remainingMessages);
					_debugStream->print(F(", CR:"));
					_debugStream->println(commitReceivedFlag);					
				}
			#endif
			
			_additionalPushMsgAvailable = false;																										// Reset the Flag for additional Messages

			if (!_sendFrame(address, 0, 0, false, true, waitForPushAnswer, commitReceivedFlag, retryOnCrFailure, payload, 0)) {	// Send the Frame
				returnValue = false;
				
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->print(F("ERR: SEND PUSH-REQUEST TO 0x"));
						_debugPrintHex(address);
						_debugStream->println(F(" FAILED!"));
					}
				#endif
			}
			
			if (!_additionalPushMsgAvailable) {																											// Break remainingMessages for, if this was the last message (-- occures bevor for checks the condition again, so this gives a zero and falls out of for...)
				remainingMessages = 1;
			}
		}
	}
	return returnValue;
}



// GetLastComError - Provides additional Informations about the Error occurred (and reset the ErrorStore)
gnMsup1::comError_t gnMsup1::getLastComError() {
	comError_t last = _lastComError;
	comError_t empty;
	_lastComError = empty;
	return last;
}



// AttachSerialDebug - Allows to output some Debug-Informations to a Serial Port (SoftwareSerial or HardwareSerial); only works if GNMSUP1_DEBUG defined
void gnMsup1::attachSerialDebug(HardwareSerial& device) {
	#ifdef GNMSUP1_DEBUG
		_hwDebugStream = &device;
		_debugStream = (Stream*)_hwDebugStream;
		_debugAttached = true;
	#endif
}

void gnMsup1::attachSerialDebug(SoftwareSerial& device) {
	#ifdef GNMSUP1_DEBUG
		_swDebugStream = &device;
		_debugStream = (Stream*)_swDebugStream;
		_debugAttached = true;
	#endif
}



// Private ///////////////////////////////////////////////////////////////////////////////////////////////////////////



// GetCallbackStoreNr -> Returns the Number of the Service in the Store; GNMSUP1_NOTINSTORE if not found
uint8_t gnMsup1::_getCallbackStoreNr(uint8_t serviceNumber) {
	for (uint8_t i = 0; i < _callbackStoreNextFree; i++) {
		if (_callbackStore[i].serviceNumber == _frameBuffer[GNMSUP1_FRAMEBUF_SERVICE]) {
			return i;
		}
	}
	return GNMSUP1_NOTINSTORE;
}



// ReadInput -> Process the Input Byte by Byte (as long as the Input-Buffer provides Bytes); Returns True, if a complete (& valid) Frame is in the Buffer
bool gnMsup1::_readInput() {
	uint8_t inputBuffer;
	if (_framePosition > 1) {																																		// Reset FramePosition if Timeout exceeded (this will cause a frame-drop)
		if (millis() - _frameStartTime > _frameTimeout) {
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->println(F("\tDROP:TIMEOUT"));
				}
			#endif
			
			_resetAndStartFrame(GNMSUP1_FRAMESTART1 + 1);																						// Reset Framebuffer with a non-StartByte (+1).
		}
	}	
	
	while (_stream->available()) {																															// Check for incoming Data
		inputBuffer = _stream->read();

		#ifdef GNMSUP1_DEBUG																																			// Initial Debug-Output, except at StopByte2!
			if (_debugAttached) {
				_debugStream->println();
				_debugStream->print(F(">0x"));
				_debugPrintHex(_framePosition);
				_debugStream->print(F("\t0x"));
				_debugPrintHex(inputBuffer);
			}
		#endif
																																															// Check the Position in Frame including Protocol Logic
		if (_framePosition == 0 && inputBuffer == GNMSUP1_FRAMESTART1) {													// StartByte 1
			_resetAndStartFrame(inputBuffer);
		}	else if (_framePosition == 1) {																													// StartByte 2
			if (inputBuffer == GNMSUP1_FRAMESTART2) {
				_frameStartTime = millis();
				_framePosition++;
			} else {																																								// StartByte 2: Invalid Data
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->println(F("\tDROP:OUTOFORDER_START1"));
					}
				#endif
				
				_resetAndStartFrame(inputBuffer);
			}
		} else if (_framePosition == 2) {																													// Flag
			_frameBuffer[GNMSUP1_FRAMEBUF_FLAG] = inputBuffer;
			_framePosition++;
			
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->print(F("\tDIR:"));
					_debugStream->print(bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_DIRECTION), BIN);
					_debugStream->print(F(", SERVICE:"));
					_debugStream->print(bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_SERVICE), BIN);
					_debugStream->print(F(", PUSH:"));
					_debugStream->print(bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_PUSH), BIN);
					_debugStream->print(F(", CR:"));
					_debugStream->print(bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_COMMITRECEIVE), BIN);
				}
			#endif
		} else if (_framePosition == 3) {																													// Address
			_frameBuffer[GNMSUP1_FRAMEBUF_ADDRESS] = inputBuffer;
			_framePosition++;
			
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->print(F("\tADDR"));
				}
			#endif
		} else if (_framePosition == 4 && 
							 bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_SERVICE)) {			// With-ServiceFlag: Payload-Length
			_frameBuffer[GNMSUP1_FRAMEBUF_PAYLOADSIZE] = inputBuffer;
			_framePosition++;
			
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->print(F("\tPAYLOAD-LEN"));
				}
			#endif
		} else if (_framePosition == 4 && 
							 !bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_SERVICE)) {		// No-ServiceFlag: Checksum High
			_frameChecksum = inputBuffer << 8;
			_framePosition++;
			
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->print(F("\tCRC16"));
				}
			#endif
		} else if (_framePosition == 5 &&
						 bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_SERVICE)) {				// With-ServiceFlag: Service
			_frameBuffer[GNMSUP1_FRAMEBUF_SERVICE] = inputBuffer;
			_framePosition++;
			
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->print(F("\tSERVICE"));
				}
			#endif
		} else if (_framePosition == 5 &&
						 !bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_SERVICE)) {			// No-ServiceFlag: Checksum Low
			_frameChecksum |= inputBuffer;
			_framePosition++;
		} else if (_framePosition == 6 && 
						 bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_SERVICE)) {				// With-ServiceFlag: Subservice
			_frameBuffer[GNMSUP1_FRAMEBUF_SUBSERVICE] = inputBuffer;
			_framePosition++;
			
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->print(F("\tSUBSERVICE"));
				}
			#endif
		} else if (_framePosition == 6 &&
						 !bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_SERVICE)) {			// No-ServiceFlag: StopByte 1
			if (inputBuffer == GNMSUP1_FRAMESTOP1) {
				_framePosition++;
				
				#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->print(F("\tSTOP"));
				}
			#endif
			} else {																																								// No-ServiceFlag: StopByte 1: Invalid Data
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->println(F("\tDROP:OUTOFORDER_STOP1NOSERVICEFLAG"));
					}
				#endif
				
				_resetAndStartFrame(inputBuffer);
			}
		} else if (_framePosition >= 7 && 																												// With-ServiceFlag: Payload
							 _framePosition < (7 + _frameBuffer[GNMSUP1_FRAMEBUF_PAYLOADSIZE]) &&
							 bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_SERVICE)) {
			_frameBuffer[GNMSUP1_FRAMEBUF_PAYLOADSTART + _framePosition - 7] = inputBuffer;
			_framePosition++;
			
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->print(F("\tPAYLOAD:0x"));
					_debugPrintHex(_framePosition - 8);
				}
			#endif
		} else if (_framePosition == 7 &&
						 !bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_SERVICE)) {			// No-ServiceFlag: StopByte 2
			if (inputBuffer == GNMSUP1_FRAMESTOP2) {
				_framePosition++;
				if (_validateFrame()) {
					return true;
				} else {
					_resetAndStartFrame(inputBuffer);
					return false;
				}
			} else {																																								// No-ServiceFlag: StopByte 2: Invalid Data
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->println(F("\tDROP:OUTOFORDER_STOP2NOSERVICEFLAG"));
					}
				#endif
				
				_resetAndStartFrame(inputBuffer);
			}
		} else if (_framePosition == (7 + _frameBuffer[GNMSUP1_FRAMEBUF_PAYLOADSIZE]) && 					// With-ServiceFlag: Checksum High
							 bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_SERVICE)) {
			_frameChecksum = inputBuffer << 8;
			_framePosition++;
			
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->print(F("\tCRC16"));
				}
			#endif
		} else if (_framePosition == (8 + _frameBuffer[GNMSUP1_FRAMEBUF_PAYLOADSIZE]) && 					// With-ServiceFlag: Checksum Low
							 bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_SERVICE)) {
			_frameChecksum |= inputBuffer;
			_framePosition++;
		} else if (_framePosition == (9 + _frameBuffer[GNMSUP1_FRAMEBUF_PAYLOADSIZE]) && 					// With-ServiceFlag: StopByte 1
							 bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_SERVICE)) {
			if (inputBuffer == GNMSUP1_FRAMESTOP1) {
				_framePosition++;
				
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->print(F("\tSTOP"));
					}
				#endif
			} else {																																								// With-ServiceFlag: StopByte 1: Invalid Data
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->println(F("\tDROP:OUTOFORDER_STOP1WITHSERVICEFLAG"));
					}
				#endif
				
				_resetAndStartFrame(inputBuffer);
			}
		} else if (_framePosition == (10 + _frameBuffer[GNMSUP1_FRAMEBUF_PAYLOADSIZE]) &&					// With-ServiceFlag: StopByte 2
							 bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_SERVICE)) {
			_framePosition++;
			if (inputBuffer == GNMSUP1_FRAMESTOP2) {
				if (_validateFrame()) {
					return true;
				} else {
					_resetAndStartFrame(inputBuffer);
					return false;
				}
			}	else {																																								// With-ServiceFlag: StopByte 2: Invalid Data
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->println(F("\tDROP:OUTOFORDER_STOP1WITHSERVICEFLAG"));
					}
				#endif
				
				_resetAndStartFrame(inputBuffer);
			}
		}	else {																																									// Invalid Data
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->println(F("\tSKIP"));
				}
			#endif
			
			_resetAndStartFrame(inputBuffer);
		}
	}
	
	return false;
}



// ProcessFrame -> Processes a incoming Frame and delegate it to the Callback or System-Service
void gnMsup1::_processFrame() {
	bool pushFlag = bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_PUSH);				// Store the Push-Flag for use after Callback
	bool additionalPushMessagesFlag;
	uint8_t empty[0];
	
	if (bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_SERVICE)) {							// Only call a Service if ServiceFlag is set
		if (_frameBuffer[GNMSUP1_FRAMEBUF_SERVICE] == GNMSUP1_SYSTEMSERVICENUMBER) {							// Checks for System-Service
			pushFlag = _handleSystemService(pushFlag);																							// Handle the System-Service, set Push-Flag dependent of the type
		} else {
			uint8_t storeEntry = _getCallbackStoreNr(_frameBuffer[GNMSUP1_FRAMEBUF_SERVICE]);
			if (storeEntry == GNMSUP1_NOTINSTORE && !_callbackCatchAllActive) {											// Check if Service or CatchAll is attached
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->print(F("ERR:SERVICE 0x"));
						_debugPrintHex(_frameBuffer[GNMSUP1_FRAMEBUF_SERVICE]);
						_debugStream->println(F(": NOT ATTACHED"));
						_debugStream->println();
					}
				#endif
			} else {																																								// Create a copy of the Payload and invoke Callback-Function
				uint8_t payload[_frameBuffer[GNMSUP1_FRAMEBUF_PAYLOADSIZE]];
				for (uint8_t i = 0; i < _frameBuffer[GNMSUP1_FRAMEBUF_PAYLOADSIZE]; i++) {
					payload[i] = _frameBuffer[GNMSUP1_FRAMEBUF_PAYLOADSTART + i];
				}
				if (storeEntry == GNMSUP1_NOTINSTORE) {
					#ifdef GNMSUP1_DEBUG
						if (_debugAttached) {
							_debugStream->println(F("INVOKE CATCHALL-CALLBACK"));
						}
					#endif
					
					_callbackCatchAllHandler(_frameBuffer[GNMSUP1_FRAMEBUF_SERVICE], _frameBuffer[GNMSUP1_FRAMEBUF_SUBSERVICE], payload, _frameBuffer[GNMSUP1_FRAMEBUF_PAYLOADSIZE], _frameBuffer[GNMSUP1_FRAMEBUF_ADDRESS]);
				} else {
					#ifdef GNMSUP1_DEBUG
						if (_debugAttached) {
							_debugStream->println(F("INVOKE SERVICE-CALLBACK"));
						}
					#endif
					
					_callbackStore[storeEntry].serviceHandler(_frameBuffer[GNMSUP1_FRAMEBUF_SUBSERVICE], payload, _frameBuffer[GNMSUP1_FRAMEBUF_PAYLOADSIZE], _frameBuffer[GNMSUP1_FRAMEBUF_ADDRESS]);
				}
				
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->println(F("CALLBACK FINISHED"));
						_debugStream->println();
					}
				#endif
			}
		}
	}
		
	// Attention: FrameBuffer maybe invalid at this point, because the callback invokes other sendPackage Functions!
	_framePosition = 0;																																					// Reset FramePosition after processing the Frame
	if (_ownsMasterRole()) {																																		// Master: Release the PushBlocking and set _additionalPushMsgAvailable Flag
		_pushBlockingRelease();
		_additionalPushMsgAvailable = pushFlag;
	} else if (pushFlag) {																																			// Check if Slave got push-clearance (as a Slave)
		if (millis() - _frameStartTime <= GNMSUP1_PUSHTIMEOUT) {																	// Assure there was no Timeout (during Callback-Function)
			uint8_t pushStoreEntry = _pushStoreNextToSend();
			if (pushStoreEntry != GNMSUP1_PUSHSTOREEMPTY) {																					// Check for PushMessages in Store and send if available
				bitWrite(_pushStore[pushStoreEntry].flags, GNMSUP1_PUSHBUFFLAG_PENDING, false);				// Mark this Message as completed
				
				additionalPushMessagesFlag = (_pushStoreNextToSend() != GNMSUP1_PUSHSTOREEMPTY);			// Calculate after Callback
				
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->print(F("PUSH-CLEARANCE RECEIVED: SEND A PENDING MESSAGE, "));
						if (additionalPushMessagesFlag) {
							_debugStream->println(F("MORE IN QEUE."));
						} else {
							_debugStream->println(F("LAST IN QEUE."));
						}
						_debugStream->println();
					}
				#endif
				
				if (!(_sendFrame(_address, _pushStore[pushStoreEntry].serviceNumber, 									// Send Frame; if NOT succeeded and CommitReceive-Flag set, mark the entry as pending again
													_pushStore[pushStoreEntry].subserviceNumber, true, 
													additionalPushMessagesFlag, false, 
													bitRead(_pushStore[pushStoreEntry].flags, GNMSUP1_PUSHBUFFLAG_COMMITRECEIVE), false, 
													_pushStore[pushStoreEntry].payload, _pushStore[pushStoreEntry].payloadSize)) && 
							bitRead(_pushStore[pushStoreEntry].flags, GNMSUP1_PUSHBUFFLAG_COMMITRECEIVE)) {
					#ifdef GNMSUP1_DEBUG
						if (_debugAttached) {
							_debugStream->println(F("ERR: REQUEUE MESSAGE DUE TO COMMITRECEIVE ERROR."));
							bitWrite(_pushStore[pushStoreEntry].flags, GNMSUP1_PUSHBUFFLAG_PENDING, true);
						}
					#endif
				}
				
			} else {																																								// Let the master know, that we dont have any PushMessages for him
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->println(F("NO PUSH MESSAGES, INFORM MASTER"));
					}
				#endif
				
				_sendFrame(_address, 0, 0, false, false, false, false, false, empty, 0);
			}
		} else {																																									// Timeout - not able to answer the Push-Request
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->println(F("WRN:TIMEOUT UNABLE TO ANSWER PUSH-REQUEST IN TIME. AVOID THIS CONDITION!"));
				}
			#endif
		}
	}
}



// ResetAndStartFrame -> Checks if StartByte received and cleans the FrameBuffer
void gnMsup1::_resetAndStartFrame(uint8_t inputBuffer) {
	if (inputBuffer == GNMSUP1_FRAMESTART1) {
		_framePosition = 1;
		_resetFramebuffer();
		
		#ifdef GNMSUP1_DEBUG
			if (_debugAttached) {
				_debugStream->print(F("\tSTART"));
			}
		#endif
	} else {
		_framePosition = 0;
	}
}



// ResetFramebuffer -> Resets/Cleans the FrameBuffer
void gnMsup1::_resetFramebuffer() {
	memset(_frameBuffer, 0, sizeof(_frameBuffer));
	_frameChecksum = 0;
}



// ValidateFrame -> Returns true if Frame is addressed to myself and has a valid Checksum; Handles also the CommitReceive Flag
bool gnMsup1::_validateFrame() {
	#ifdef GNMSUP1_DEBUG
		if (_debugAttached) {
			_debugStream->println();
		}
	#endif
	
	if (_frameChecksum != CRC16.ccitt(_frameBuffer, (_framePosition - 6))) {										// CRC16 Validation
		#ifdef GNMSUP1_DEBUG
			if (_debugAttached) {
				_debugStream->println(F("DROP:INVALID CHECKSUM"));
			}
		#endif
		
		return false;
	}
	
	_markActive(_frameBuffer[GNMSUP1_FRAMEBUF_ADDRESS]);																				// Mark Address as active
	
	if (_address != _frameBuffer[GNMSUP1_FRAMEBUF_ADDRESS] &&																		// Address Filter
			!_ownsMasterRole()) {
		#ifdef GNMSUP1_DEBUG
			if (_debugAttached) {
				_debugStream->println(F("DROP:NOT FOR ME"));
			}
		#endif
		
		return false;
	}
	
	if (!_ownsMasterRole() &&																																		// Direction Filter
			bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_DIRECTION)) {
		#ifdef GNMSUP1_DEBUG
			if (_debugAttached) {
				_debugStream->println(F("ERR:DUPLICATE ADDRESS DETECTED"));
			}
		#endif
		
		return false;				
	}
	
	if (bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_COMMITRECEIVE)) {				// CommitReceive handler
		if (_hwLayer == RS485) {																																	// Set DE for RS485; delay Debug-Output because its time-sensitive
			digitalWrite(_rs485DePin, HIGH);
			delayMicroseconds(GNMSUP1_RS485_DEENABLEWAITMICROS);
		}
		
		_stream->write(highByte(_frameChecksum));																									// Write-Out CRC16
		_stream->write(lowByte(_frameChecksum));
		
		_stream->flush();																																					// Wait till written-Out
	
		if (_hwLayer == RS485) {																																	// Release DE for RS485
			digitalWrite(_rs485DePin, LOW);
		}
		
		#ifdef GNMSUP1_DEBUG																																			// Now write all Debug-Informations
			if (_debugAttached) {
				if (_hwLayer == RS485) {
					_debugStream->println(F("<RS485 DE-PIN SET"));
				}
				_debugStream->print(F("<0x00\t0x"));
				_debugPrintHex(highByte(_frameChecksum));
				_debugStream->println(F("\tCOMMITRECEIVE"));
				_debugStream->print(F("<0x01\t0x"));
				_debugPrintHex(lowByte(_frameChecksum));
				_debugStream->println();
				if (_hwLayer == RS485) {
					_debugStream->println(F("<RS485 DE-PIN RELEASED"));
				}
			}
		#endif
	}
	
	#ifdef GNMSUP1_DEBUG
		if (_debugAttached) {
			_debugStream->println(F("FRAME:VALID"));
		}
	#endif
				
	return true;
}



// HandleSystemService -> Process a System-Service Frame
bool gnMsup1::_handleSystemService(bool pushFlag) {
	bool pushAnswerCommitReceiveFlag = bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_COMMITRECEIVE);	// Evaluate the CommitReceive-Flag
	uint8_t empty[0];
	
	if (_ownsMasterRole()) {																																		// Role specific implementations
		switch (_frameBuffer[GNMSUP1_FRAMEBUF_SUBSERVICE]) {
			case GNMSUP1_SYSTEMSERVICE_IGNORE:																											// Ignore-Service (ignore :-)
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->println(F("GOT A IGNORE-PACKET"));
					}
				#endif
				return pushFlag;
				
			default:																																								// Not implemented
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->print(F("ERR:SYSTEM-SERVICE 0x"));
						_debugPrintHex(_frameBuffer[GNMSUP1_FRAMEBUF_SUBSERVICE]);
						_debugStream->println(F(": NOT IMPLEMENTED ON MASTER"));
						_debugStream->println();
					}
				#endif
				return pushFlag;
		}
	} else {																																										// Slave Role, processing System-Service...
		bool additionalPushMessagesFlag = (_pushStoreNextToSend() != GNMSUP1_PUSHSTOREEMPTY);			// Calculate the PushFlag
		
		switch (_frameBuffer[GNMSUP1_FRAMEBUF_SUBSERVICE]) {
			case GNMSUP1_SYSTEMSERVICE_QUERYALIVE:																									// QueryAlive, send Answer
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->println(F("ASKED TO SEND ALIVE MESSAGE"));
					}
				#endif
				_sendFrame(_address, GNMSUP1_SYSTEMSERVICENUMBER, GNMSUP1_SYSTEMSERVICE_QUERYALIVE, true, additionalPushMessagesFlag, false, pushAnswerCommitReceiveFlag, false, empty, 0);
				return false;
				
			case GNMSUP1_SYSTEMSERVICE_IGNORE:																											// Ignore-Service (ignore :-)
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->println(F("GOT A IGNORE-PACKET"));
					}
				#endif
				return pushFlag;
			
			default:																																								// Not implemented
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->print(F("ERR:SYSTEM-SERVICE 0x"));
						_debugPrintHex(_frameBuffer[GNMSUP1_FRAMEBUF_SUBSERVICE]);
						_debugStream->println(F(": NOT IMPLEMENTED"));
						_debugStream->println();
					}
				#endif
				return pushFlag;
		}
	}
}



// PushBlockingWaitForRelease -> Waits until Push-Answer received or Push-Request timeouted (only call after prechecking Mode and _pushBlockingActive()!)
void gnMsup1::_pushBlockingWaitForRelease() {
	#ifdef GNMSUP1_DEBUG
		if (_debugAttached) {
			_debugStream->println(F("WRN: PUSH-ANSWER OPEN; MODE ENFORCE BLOCKING WAIT."));
		}
	#endif
	
	while (_pushBlockingActive()) {
		handleCommunication();
	}
}



// PushStoreStoreNextToSend -> Gets the Slot with the oldest entry (not timeouted); returns GNMSUP1_PUSHSTOREEMPTY if empty
uint8_t	gnMsup1::_pushStoreNextToSend() {
	uint8_t storeEntry = GNMSUP1_PUSHSTOREEMPTY;
	uint32_t millisbuffer = millis();
	uint32_t timestamp = millisbuffer;
	
	for (uint8_t i = 0; i < GNMSUP1_MAXPUSHQEUEENTRYS; i++) {
		if (bitRead(_pushStore[i].flags, GNMSUP1_PUSHBUFFLAG_PENDING) && 
				millisbuffer - _pushStore[i].timestamp <= GNMSUP1_DEFAULTPUSHQEUETIMEOUT &&
				_pushStore[i].timestamp < timestamp) {
			storeEntry = i;
		}
	}
	
	return storeEntry;
}



// PushStoreStoreNextFree -> Gets the next Slot that is free (timeouted); returns GNMSUP1_PUSHSTOREFULL if full
uint8_t	gnMsup1::_pushStoreNextFree() {
	uint32_t millisbuffer = millis();
	
	for (uint8_t i = 0; i < GNMSUP1_MAXPUSHQEUEENTRYS; i++) {
		if (!(bitRead(_pushStore[i].flags, GNMSUP1_PUSHBUFFLAG_PENDING)) ||
				millisbuffer - _pushStore[i].timestamp > GNMSUP1_DEFAULTPUSHQEUETIMEOUT) {
			return i;
		}
	}
	return GNMSUP1_PUSHSTOREFULL;
}



// SendFrame -> Internal Send Function
bool gnMsup1::_sendFrame(uint8_t address, uint8_t serviceNumber, uint8_t subserviceNumber, bool serviceFlag, bool pushFlag, bool waitForPushAnswer, bool commitReceivedFlag, bool retryOnCrFailure, uint8_t payload[], uint8_t payloadSize) {
	if (_queryIgnore(address)) {																																// Check if this Node should be ignored
		#ifdef GNMSUP1_DEBUG
			if (_debugAttached) {
				_debugStream->print(F("WRN: ADDRESS 0x"));
				_debugPrintHex(address);
				_debugStream->println(F(" IS INACTIVE, SKIP SEND-REQUEST."));
			}
		#endif
		return false;
	}
	
	if (_hwLayer == RS485) {																																		// Set DE for RS485
		digitalWrite(_rs485DePin, HIGH);
		delayMicroseconds(GNMSUP1_RS485_DEENABLEWAITMICROS);
		#ifdef GNMSUP1_DEBUG
			if (_debugAttached) {
				_debugStream->println(F("<RS485 DE-PIN SET"));
			}
		#endif
	}
	
	_resetFramebuffer();																																				// Reset Framebuffer and build the Frame
	uint16_t framebufferLength = 0;
	uint8_t flagbyte = 0x00;
	bitWrite(flagbyte, GNMSUP1_FRAMEFLAG_DIRECTION, (!_ownsMasterRole()));
	bitWrite(flagbyte, GNMSUP1_FRAMEFLAG_SERVICE, serviceFlag);
	bitWrite(flagbyte, GNMSUP1_FRAMEFLAG_PUSH, pushFlag);
	bitWrite(flagbyte, GNMSUP1_FRAMEFLAG_COMMITRECEIVE, commitReceivedFlag);
	_frameBuffer[GNMSUP1_FRAMEBUF_FLAG] = flagbyte;
	_frameBuffer[GNMSUP1_FRAMEBUF_ADDRESS] = address;
	if (serviceFlag) {
		framebufferLength = 5 + payloadSize;
		_frameBuffer[GNMSUP1_FRAMEBUF_PAYLOADSIZE] = payloadSize;
		_frameBuffer[GNMSUP1_FRAMEBUF_SERVICE] = serviceNumber;
		_frameBuffer[GNMSUP1_FRAMEBUF_SUBSERVICE] = subserviceNumber;
		for (uint8_t i = 0; i < payloadSize; i++) {
			_frameBuffer[GNMSUP1_FRAMEBUF_PAYLOADSTART + i] = payload[i];
		}
	} else {
		framebufferLength = 2;
	}
	_frameChecksum = CRC16.ccitt(_frameBuffer, framebufferLength);

	_stream->write(GNMSUP1_FRAMESTART1);																												// Write-Out StartBytes, FrameBuffer, StopBytes and wait for complete...
	#ifdef GNMSUP1_DEBUG
		if (_debugAttached) {
			_debugStream->print(F("<0x00\t0x"));
			_debugPrintHex(GNMSUP1_FRAMESTART1);
			_debugStream->println(F("\tSTART"));
		}
	#endif
	_stream->write(GNMSUP1_FRAMESTART2);
	#ifdef GNMSUP1_DEBUG
		if (_debugAttached) {
			_debugStream->print(F("<0x01\t0x"));
			_debugPrintHex(GNMSUP1_FRAMESTART2);
			_debugStream->println();
		}
	#endif
	
	if (_ownsMasterRole() && pushFlag) {																												// Set the Push-Blocker (only as Master relevant)
		_pushBlockingSet();
	}
	
	_stream->write(_frameBuffer, framebufferLength);
	#ifdef GNMSUP1_DEBUG
		if (_debugAttached) {
			for (uint8_t i = 0; i < framebufferLength; i++) {
				_debugStream->print(F("<0x"));
				_debugPrintHex(2 + i);
				_debugStream->print(F("\t0x"));
				_debugPrintHex(_frameBuffer[i]);
				switch (i) {
					case GNMSUP1_FRAMEBUF_FLAG:
						_debugStream->print(F("\tDIR:"));
						_debugStream->print(bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_DIRECTION), BIN);
						_debugStream->print(F(", SERVICE:"));
						_debugStream->print(bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_SERVICE), BIN);
						_debugStream->print(F(", PUSH:"));
						_debugStream->print(bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_PUSH), BIN);
						_debugStream->print(F(", CR:"));
						_debugStream->print(bitRead(_frameBuffer[GNMSUP1_FRAMEBUF_FLAG], GNMSUP1_FRAMEFLAG_COMMITRECEIVE), BIN);
						break;
					case GNMSUP1_FRAMEBUF_ADDRESS:
						_debugStream->print(F("\tADDR"));
						break;
					case GNMSUP1_FRAMEBUF_PAYLOADSIZE:
						_debugStream->print(F("\tPAYLOAD-LEN"));
						break;
					case GNMSUP1_FRAMEBUF_SERVICE:
						_debugStream->print(F("\tSERVICE"));
						break;
					case GNMSUP1_FRAMEBUF_SUBSERVICE:
						_debugStream->print(F("\tSUBSERVICE"));
						break;
					default:
						_debugStream->print(F("\tPAYLOAD:0x"));
						_debugPrintHex(i - GNMSUP1_FRAMEBUF_PAYLOADSTART);
				}
				_debugStream->println();
			}
		}
	#endif
	
	_stream->write(highByte(_frameChecksum));
	#ifdef GNMSUP1_DEBUG
		if (_debugAttached) {
			_debugStream->print(F("<0x"));
			_debugPrintHex(2 + framebufferLength);
			_debugStream->print(F("\t0x"));
			_debugPrintHex(highByte(_frameChecksum));
			_debugStream->println(F("\tCRC16"));
		}
	#endif
	_stream->write(lowByte(_frameChecksum));
	#ifdef GNMSUP1_DEBUG
		if (_debugAttached) {
			_debugStream->print(F("<0x"));
			_debugPrintHex(3 + framebufferLength);
			_debugStream->print(F("\t0x"));
			_debugPrintHex(lowByte(_frameChecksum));
			_debugStream->println();
		}
	#endif
	
	_stream->write(GNMSUP1_FRAMESTOP1);
	#ifdef GNMSUP1_DEBUG
		if (_debugAttached) {
			_debugStream->print(F("<0x"));
			_debugPrintHex(4 + framebufferLength);
			_debugStream->print(F("\t0x"));
			_debugPrintHex(GNMSUP1_FRAMESTOP1);
			_debugStream->println(F("\tSTOP"));
		}
	#endif
	_stream->write(GNMSUP1_FRAMESTOP2);
	_stream->flush();																																						// Timing sensitive: Hold back any Debug-Output for later and wait till all Bytes are written out...
	
	if (_hwLayer == RS485) {																																		// Release DE for RS485
		digitalWrite(_rs485DePin, LOW);
	}
	
	#ifdef GNMSUP1_DEBUG																																				// Now its time for Debug-Informations
		if (_debugAttached) {
			_debugStream->print(F("<0x"));
			_debugPrintHex(5 + framebufferLength);
			_debugStream->print(F("\t0x"));
			_debugPrintHex(GNMSUP1_FRAMESTOP2);
			_debugStream->println();
			if (_hwLayer == RS485) {
				_debugStream->println(F("<RS485 DE-PIN RELEASED"));
			}
		}
	#endif
	
	if (commitReceivedFlag) {																																		// Wait for CommitReceive if Flag is set and process the answer
		uint8_t crBuffer[2];
		_stream->setTimeout(_frameTimeout * 4 / 10);
		if (_stream->readBytes(crBuffer, 2) == 2) {
			if (crBuffer[0] == highByte(_frameChecksum) && crBuffer[1] == lowByte(_frameChecksum)) {
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->println(F("CR VALID"));
					}
				#endif
				_markActive(address);
			} else {
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->println(F("ERR: CR INVALID"));
					}
				#endif
				_lastComError.comErrorCode = gnMsup1::Err_CRInvalid;
				_lastComError.address = address;
				if (retryOnCrFailure && _ownsMasterRole()) {																					// Allow master to retry, if requested
					#ifdef GNMSUP1_DEBUG
						if (_debugAttached) {
							_debugStream->println(F("RETRY..."));
						}
					#endif
					return _sendFrame(address, serviceNumber, subserviceNumber, serviceFlag, pushFlag, waitForPushAnswer, commitReceivedFlag, false, payload, payloadSize);
				} else {
					return false;
				}
			}
		} else {
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->println(F("ERR: CR TIMEOUT"));
				}
			#endif
			_lastComError.comErrorCode = gnMsup1::Err_CRTimeout;
			_lastComError.address = address;
			
			if (retryOnCrFailure && _ownsMasterRole()) {																					// Allow master to retry, if requested
				#ifdef GNMSUP1_DEBUG
					if (_debugAttached) {
						_debugStream->println(F("RETRY..."));
					}
				#endif
				return _sendFrame(address, serviceNumber, subserviceNumber, serviceFlag, pushFlag, waitForPushAnswer, commitReceivedFlag, false, payload, payloadSize);
			} else {
				_markIgnore(address);
				return false;
			}
		}
	}
	
	if (_ownsMasterRole() && pushFlag && waitForPushAnswer) {																		// If requested, wait for Answer or Timeout (only Push-Requests form the Master)
		_pushBlockingWaitForRelease();
	}

	return true;
}



// Query if a Node shoud be ignored (on Slaves return always false, if IgnoreInactiveNodes is off return always false)
bool gnMsup1::_queryIgnore(uint8_t address) {
	if (!(_ownsMasterRole()) || !(_ignoreInactiveNodes)) {																			// Shortcut for Slaves or inactive IgnoreInactiveNodes Mode
		return false;
	} else if (address > GNMSUP1_MAXSLAVEADDRESS) {																							// Ignore invalid Addresses
		return false;
	} else {																																										// Return IgnoreStore Value
		return bitRead(_ignoreStore[(address / 8)], (address % 8));
	}
}



// Query if a Node is known as active (if IgnoreInactiveNodes is off return always true)
bool gnMsup1::_queryActive(uint8_t address) {
	if (!(_ignoreInactiveNodes)) {																															// Shortcut while inactive IgnoreInactiveNodes Mode
		return true;
	} else if (address > GNMSUP1_MAXSLAVEADDRESS) {																							// Ignore invalid Addresses
		return false;
	} else {																																										// Return IgnoreStore Value
		return bitRead(_activeStore[(address / 8)], (address % 8));
	}
}



// Mark a Node as Active (don't ignore him)
void gnMsup1::_markActive(uint8_t address) {
	if (address <= GNMSUP1_MAXSLAVEADDRESS) {																										// Ignore invalid Addresses
		bitWrite(_activeStore[(address / 8)], (address % 8), 1);
		bitWrite(_ignoreStore[(address / 8)], (address % 8), 0);
	}
}



// Mark a Node as Inactive (ignore him until next Scan)
void gnMsup1::_markIgnore(uint8_t address) {
	if (address <= GNMSUP1_MAXSLAVEADDRESS) {																										// Ignore invalid Addresses
		bitWrite(_activeStore[(address / 8)], (address % 8), 0);
		bitWrite(_ignoreStore[(address / 8)], (address % 8), 1);
	}
}



// Reset the Mark for a active or ignored Node
void gnMsup1::_resetActiveIgnore(uint8_t address) {
	if (address <= GNMSUP1_MAXSLAVEADDRESS) {																										// Ignore invalid Addresses
		bitWrite(_activeStore[(address / 8)], (address % 8), 0);
		bitWrite(_ignoreStore[(address / 8)], (address % 8), 0);
	}
}



// Scavenging inactive-Node Store (when needed)
void gnMsup1::_scavengingInactive() {
	if (_ignoreInactiveNodes) {																																	// Proceed only when... IgnoreInactiveNodes active
		if (millis() - _scavengingInactiveLastTimestamp > GNMSUP1_SCAVENGINGINACTIVEINTERVAL) {		// ... and Interval necessary
			_scavengingInactiveLastTimestamp = millis();
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->println(F("SCAVENGING INACTIVE NODES NOW..."));
				}
			#endif
	
			bool scavengingOk = false;
			uint8_t address = _scavengingInactiveNextAddress;
			while (!scavengingOk) {																																	// Start scavenging where we ended last time, process up from here every active Node and one inactive Node...
				if (_queryActive(address)) {																													// If node already known as active, reset its status and goto next address
					_resetActiveIgnore(address);
					
				} else if (_queryIgnore(address)) {																										// If node is currently ingored, rescan him
					scavengingOk = true;
					
					#ifdef GNMSUP1_DEBUG
						if (_debugAttached) {
							_debugStream->print(F("RESET 0x"));
							_debugPrintHex(address);
							_debugStream->println(F(" AND SEND QUERYALIVE"));
						}
					#endif
					
					uint8_t empty[0];
					_resetActiveIgnore(address);
					comError_t lastComError = _lastComError;
					if (_sendFrame(address, GNMSUP1_SYSTEMSERVICENUMBER, GNMSUP1_SYSTEMSERVICE_IGNORE, true, false, false, true, false, empty, 0)) {
						_markActive(address);
						#ifdef GNMSUP1_DEBUG
							if (_debugAttached) {
								_debugStream->print(F("SCAVENING: REACTIVATED NODE"));
							}
						#endif
					
					} else {
						_markIgnore(address);
						#ifdef GNMSUP1_DEBUG
							if (_debugAttached) {
								_debugStream->print(F("SCAVENING: NODE STILL INACTIVE"));
							}
						#endif						
					}
					_lastComError = lastComError;
				}
				
				address++;																																						// Increment address, avoid overflow and break if we checked all nodes
				if (address > GNMSUP1_MAXSLAVEADDRESS) {
					address = 0;
				}
				if (address == _scavengingInactiveNextAddress) {
					scavengingOk = true;
				}
			}
			
			_scavengingInactiveNextAddress = address;																								// Store the address for next run
			
			#ifdef GNMSUP1_DEBUG
				if (_debugAttached) {
					_debugStream->println(F("SCAVENGING ENDED."));
					_debugStream->println();
				}
			#endif
			/*
			for (uint8_t i = 0; i < GNMSUP1_MAXSLAVEADDRESS + 1; i++) {															// Bitwise Flip the active-store
				_ignoreStore[i] = ~_activeStore[i];																										// Bitwise Flip the active-store
			}*/
		}
	}
}



#ifdef GNMSUP1_DEBUG
	// DebugPrintHex -> Print a formatted hex-number (0x01 instead of 0x1)
	void gnMsup1::_debugPrintHex(uint8_t value) {
		if (_debugAttached) {
			if (value < 0x10) {
				_debugStream->print(0);
			}
			_debugStream->print(value, HEX);
		}
	}
#endif