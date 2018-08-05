/*  GN Master Slave Universal Protocol Library
 *  ==========================================
 *  
 *  Library for generic Master/Slave Communications.
 *  
 *  THIS IS THE USER-CONFIG FILE.
 *	Edit this File to customize the Protocol to you requirements.
 *  
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

 

#define GNMSUP1_DEBUG																						// Uncomment to get useful debug-output by use of attachSerialDebug. But debugging produces much code overhead, affecting program size and speed
#define GNMSUP1_DEFAULTBLOCKING							NearlyAsynchronous	// See Readme
#define GNMSUP1_DEFAULTBAUDRATE							9600								// Default Bitrate (can be overwritten in the begin call)
#define GNMSUP1_MAXPAYLOADBUFFER						8										// Uses Memory, align to maximum Payload-Size
#define GNMSUP1_FRAMELENGHTTIMEOUT					30									// Defines the Frameout Time as N-Times of (ideal) FrameTime. Use only integer Values. To aggressive Values produce Drops on slow Systems. To conservative Values reduces in excessive waits on transmission errors. In an ideal world you can use 1. Thats very aggressive. When one Node has Debug enabled, i recommend to use at least 25. With 30 you are in a save Area and you still have low Performance impact. These Values are valid for 115200 Baud (RS485 AND Debug-Port Speet). Slower Baudrates allow to use lower Factors, because the Node has more time to process between two Bytes.
#define GNMSUP1_DEFAULTPUSHQEUETIMEOUT			20000ul							// Timeout in Milliseconds for a Pushmessage to stay in qeue
#define GNMSUP1_MAXPUSHQEUEENTRYS						10									// Deep of the Push-Qeue; uses n * (8 + GNMSUP1_MAXPAYLOADBUFFER) Bytes; 254 max
#define GNMSUP1_MAXSERVICECOUNT							10									// 3 Bytes per Service
#define GNMSUP1_PUSHTIMEOUT									50									// Time in ms for a Slave to answer a Push-Request; Measured after sending the StartBytes of the PushRequest (so include time for process the potential payloaded Frame, prior to answer the push request.)
#define GNMSUP1_MAXSLAVEADDRESS							0x1F								// Limits the maximum Count of Slaves to reduce memory requirements of the library
#define GNMSUP1_SCAVENGINGINACTIVEINTERVAL	10000								// Period after inactive (ignored) Nodes would be rescanned	