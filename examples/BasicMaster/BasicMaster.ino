/*  GN Master Slave Universal Protocol Library - Exaple: BasicMaster
 *  ================================================================
 *  
 *  Library for generic Master/Slave Communications.
 *  This is a simple Example of a Slave that just send out some Commands.
 *  
 *  Tested with Arduino UNO and RS485 BUS.
 *  
 *  2018-08-24  V1.1.1		Andreas Gloor            SourceAddress Parameter in Callback Function
 *  2018-07-21  V1.0.1    Andreas Gloor            Initial Version
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

#include <gnMsup1.h>                                                                                                  // Include MSUP Library
#include <SoftwareSerial.h>                                                                                           // Used for Debug-Output



uint32_t delayTimer = 0;                                                                                              // Use Timer based Delays instead of delay()!
gnMsup1 master(Serial, gnMsup1::RS485, 2, gnMsup1::Master);                                                           // Setup MSUP Master
SoftwareSerial devSerialDebug(A0, A1);                                                                                // Setup Debug-Output as SoftwareSerial (HardwareSerial also supported!)



void setup() {
  devSerialDebug.begin(115200);                                                                                       // Initialize Debug-Output
  devSerialDebug.println();
  devSerialDebug.println();
  devSerialDebug.println();
  devSerialDebug.println();
  devSerialDebug.println();
  devSerialDebug.println(F("Master started"));
  devSerialDebug.println(F("=============="));
  devSerialDebug.println();
  devSerialDebug.println();
  master.attachSerialDebug(devSerialDebug);                                                                           // Attach Debug-Output to MSUP

  master.begin(115200);                                                                                               // Start the MSUP Slave
}



void loop() {
  master.handleCommunication();                                                                                       // Call the MSUP handleCommunication as much as possible; Avoid long delay() Calls!

  if (millis() - delayTimer > 600) {                                                                                  // Aequivalent to a delay(600), but non-blocking!
    uint8_t address = random(0, 2);                                                                                   // Define the Parameter-Values, using some random Values...
    uint8_t serviceNumber = 0x10;
    uint8_t subserviceNumber = 0x00;
    uint8_t payload[3] = {byte(random(0, 255)), byte(random(0, 255)), byte(random(0, 255))};
    uint8_t payloadSize = 3;
    master.send(address, serviceNumber, subserviceNumber, payload, payloadSize);
    delayTimer = millis();
  }
}



// DebugPrintHex -> Print a formatted hex-number (0x01 instead of 0x1)
void debugPrintHex(uint8_t value) {
  if (value < 0x10) {
    devSerialDebug.print(0);
  }
  devSerialDebug.print(value, HEX);
}
