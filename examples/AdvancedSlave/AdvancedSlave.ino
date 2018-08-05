/*  GN Master Slave Universal Protocol Library - Exaple: AdvanceSlave
 *  =================================================================
 *  
 *  Library for generic Master/Slave Communications.
 *  This is a simple Example of a advanced Slave that receives Commands and also triggers push-messages.
 *  
 *  Tested with Arduino UNO and RS485 BUS.
 *  
 *  2018-07-21  V1.0.1    Andreas Gloor            Initial Version
 *  
 *  MIT License
 *  
 *  Copyright (c) 2018 Andreas Gloor
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */


#define SLAVE_ADDRESS_MSB   0x00                                                                                      // Define a individual Address for each Slave; Maybe you like to use EEPROM to store, or DIP-Switches to dynamic configure...
#define PIN_ADDR0           A5                                                                                        // This Example OR's this Pin to the LSB of the Slave-Address. So you can easy configure two Slaves by Hardware.
#define PIN_DBGRX           A0                                                                                        // SoftwareSerial for Debug-Messages; Use a TTL-COM Adapter or another Arduino to read it; This is optional!
#define PIN_DBGTX           A1
#define PIN_RS485DE         2                                                                                         // DataEnable Pin for RS485
#define PIN_LEDR            3                                                                                         // Attach RGB LED (or single LED's) to this Pins to "see" the example; Connect to GND
#define PIN_LEDG            5
#define PIN_LEDB            6
#define PIN_BTNR            8                                                                                         // Attach Push-Buttons (RGB) to this Pins to "touch" the example; Connect to GND
#define PIN_BTNG            9
#define PIN_BTNB            10



#include <gnMsup1.h>                                                                                                  // Include MSUP Library
#include <SoftwareSerial.h>                                                                                           // Used for Debug-Output
#include <Bounce2.h>                                                                                                  // This Example uses a Library for debouncing the Input-Pins: Bounce2 by Thomas O Fredericks, V2.42, MIT License, https://github.com/thomasfredericks/Bounce2



gnMsup1 slave(Serial, gnMsup1::RS485, PIN_RS485DE, gnMsup1::Slave);                                                   // Setup MSUP as a Slave
SoftwareSerial devSerialDebug(PIN_DBGRX, PIN_DBGTX);                                                                  // Setup Debug-Output as SoftwareSerial (HardwareSerial also supported!)
Bounce debouncer[3];                                                                                                  // Define the debouncers
uint8_t rgbValue[3] = {0, 0, 0};                                                                                      // Only needed for the second example


void setup() {
  pinMode(PIN_ADDR0, INPUT_PULLUP);                                                                                   // Initialize Address-Pin
  pinMode(PIN_LEDR, OUTPUT);                                                                                          // Initialize LED Pins as Output (not neccessary for DE-Pin!)
  pinMode(PIN_LEDG, OUTPUT);
  pinMode(PIN_LEDB, OUTPUT);
  pinMode(PIN_BTNR, INPUT_PULLUP);                                                                                    // Initialize Push-Button Pins as Input with a Pullup and initialize the Debouncer
  pinMode(PIN_BTNG, INPUT_PULLUP);
  pinMode(PIN_BTNB, INPUT_PULLUP);
  debouncer[0].interval(10);
  debouncer[0].attach(PIN_BTNR);
  debouncer[1].interval(10);
  debouncer[1].attach(PIN_BTNG);
  debouncer[2].interval(10);
  debouncer[2].attach(PIN_BTNB);
  
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

  uint8_t addr = SLAVE_ADDRESS_MSB | digitalRead(PIN_ADDR0);                                                          // Get's the Address using Address-Pin and Configured Base-Address.
  slave.begin(115200, addr);                                                                                          // Start the MSUP Slave
}



void loop() {
  slave.handleCommunication();                                                                                        // Call the MSUP handleCommunication as much as possible; Avoid long delay() Calls!

  
  
  // 1st Example
  for (uint8_t i = 0; i < 3; i++) {                                                                                   // Check each of the Push-Buttons for press; See Bounce2 documentation for details: https://github.com/thomasfredericks/Bounce2/wiki
    debouncer[i].update();
    if (debouncer[i].fell()) {
      if (slave.push(0x11, i)) {                                                                                      // Send a Push-Message by MSUP to Service 0x11, using Subservice as carrier for the button
        devSerialDebug.println(F("AdvancedSlave: Push ok"));
      } else {
        devSerialDebug.println(F("AdvancedSlave: Push failed!"));
      }
    }
  }


  /*
  // 2nd Example: A alternate way to implement this service woud be something like this:
  bool sendTrigger = false;                                                                                           // These lines use a 3-byte payload instead of the subservice
  for (uint8_t i = 0; i < 3; i++) {                                                                                   // Its more logic needed and the behavior is different from the one above
    if (debouncer[i].update()) {
      sendTrigger = true;
      devSerialDebug.print(i);
      if (debouncer[i].fell()) {
        rgbValue[i] = 0xFF;
      } else {
        rgbValue[i] = 0x00;
      }
    }
  }
  if (sendTrigger) {
    slave.push(0x12, 0, rgbValue, 3);
  }
  */

  
  // Do something else, if its much below timeout of a packet...
}



// Sample Function Handler
void handleLed(uint8_t subserviceNumber, uint8_t payload[], uint8_t payloadCount) {
  devSerialDebug.print(F("This is LED Service Callback for Subservice:0x"));                                          // Write some Debug-Output (you can use the same Interface for your own Output)
  devSerialDebug.print(subserviceNumber, HEX);
  devSerialDebug.print(F("\t Payload:"));
  for (uint8_t i = 0; i < payloadCount; i++) {
    devSerialDebug.print(F(" 0x"));
    debugPrintHex(payload[i]);
  }
  devSerialDebug.println();

  if (subserviceNumber == 0x00 && payloadCount == 3) {                                                                // Payload-Format of this Test-Service is: RGB
    analogWrite(PIN_LEDR, payload[0]);
    analogWrite(PIN_LEDG, payload[1]);
    analogWrite(PIN_LEDB, payload[2]);
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
