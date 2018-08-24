/*  GN Master Slave Universal Protocol Library - Exaple: BasicSlave
 *  ===============================================================
 *  
 *  Library for generic Master/Slave Communications.
 *  This is a simple Example of a (passive) Slave that receives Commands without active sending Data.
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


#define SLAVE_ADDRESS       0x00                                                                                      // Define a individual Address for each Slave; Maybe you like to use EEPROM to store, or DIP-Switches to dynamic configure...
#define PIN_DBGRX           A0                                                                                        // SoftwareSerial for Debug-Messages; Use a TTL-COM Adapter or another Arduino to read it; This is optional!
#define PIN_DBGTX           A1
#define PIN_RS485DE         2                                                                                         // DataEnable Pin for RS485
#define PIN_R               3                                                                                         // Attach RGB LED (or single LED's) to this Pins to "see" the example
#define PIN_G               5
#define PIN_B               6

#include <gnMsup1.h>                                                                                                  // Include MSUP Library
#include <SoftwareSerial.h>                                                                                           // Used for Debug-Output



gnMsup1 slave(Serial, gnMsup1::RS485, PIN_RS485DE, gnMsup1::Slave);                                                   // Setup MSUP as a Slave
SoftwareSerial devSerialDebug(PIN_DBGRX, PIN_DBGTX);                                                                  // Setup Debug-Output as SoftwareSerial (HardwareSerial also supported!)



void setup() {
  pinMode(PIN_R, OUTPUT);                                                                                             // Initialize LED Pins as Output (not neccessary for DE-Pin!)
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  
  devSerialDebug.begin(115200);                                                                                       // Initialize Debug-Output
  devSerialDebug.println();
  devSerialDebug.println();
  devSerialDebug.println();
  devSerialDebug.println();
  devSerialDebug.println();
  devSerialDebug.println(F("Slave started"));
  devSerialDebug.println(F("============="));
  devSerialDebug.println();
  devSerialDebug.println();
  slave.attachSerialDebug(devSerialDebug);                                                                            // Attach Debug-Output to MSUP (Optional!)

  slave.attachService(0x10, handleLed);                                                                               // Attach a Service (Nr. 0x10) to MSUP; Use a Handler-Function to provide as a Callback

  slave.begin(115200, SLAVE_ADDRESS);                                                                                 // Start the MSUP Slave
}



void loop() {
  slave.handleCommunication();                                                                                        // Call the MSUP handleCommunication as much as possible; Avoid long delay() Calls!
  // do something else, if its much below timeout...
}



// Sample Function Handler
void handleLed(uint8_t subserviceNumber, uint8_t payload[], uint8_t payloadCount, uint8_t sourceAddress) {
  devSerialDebug.print(F("This is LED Service Callback for Subservice:0x"));                                          // Write some Debug-Output (you can use the same Interface for your own Output)
  devSerialDebug.print(subserviceNumber, HEX);
  devSerialDebug.print(F("\t Payload:"));
  for (uint8_t i = 0; i < payloadCount; i++) {
    devSerialDebug.print(F(" 0x"));
    debugPrintHex(payload[i]);
  }
  devSerialDebug.println();

  if (subserviceNumber == 0x00 && payloadCount == 3) {                                                                // Payload-Format of this Test-Service is: RGB
    analogWrite(PIN_R, payload[0]);
    analogWrite(PIN_G, payload[1]);
    analogWrite(PIN_B, payload[2]);
    devSerialDebug.println(F("Written to RGB LED."));
  } else {
    devSerialDebug.println(F("Error: Subservice unknown or PayloadCount invalid!"));
  }  
}



// DebugPrintHex -> Print a formatted hex-number (0x01 instead of 0x1)
void debugPrintHex(uint8_t value) {
  if (value < 0x10) {
    devSerialDebug.print(0);
  }
  devSerialDebug.print(value, HEX);
}
