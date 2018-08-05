/*  GN Master Slave Universal Protocol Library - Exaple: BasicMaster
 *  ================================================================
 *  
 *  Library for generic Master/Slave Communications.
 *  This is a simple Example of a Slave that just send out some Commands.
 *  
 *  Tested with Arduino UNO or MEGA2650 and RS485 BUS.
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

#define GNMSUP1_CONFIG_DEBUG
#include <gnMsup1.h>                                                                                                  // Include MSUP Library

#define devSerialDebug  Serial
/* Alternate Way if you run out of Serial Ports :-)
#include <SoftwareSerial.h>                                                                                           // Used for Debug-Output
SoftwareSerial devSerialDebug(A0, A1);                                                                                // Setup Debug-Output as SoftwareSerial (HardwareSerial also supported!)
*/


uint32_t delayTimer = 0;                                                                                              // Use Timer based Delays instead of delay()!
gnMsup1 master(Serial1, gnMsup1::RS485, 2, gnMsup1::Master);                                                          // Setup MSUP Master
uint8_t rgbValue[3] = {0, 0, 0};                                                                                      // To store actual RGB values (and send them to Slaves)



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

  master.attachService(0x11, handleLed);                                                                              // Attach a Service (Nr. 0x11) to MSUP; Use a Handler-Function to provide as a Callback
  master.attachCatchAllService(handleAll);                                                                            // Attach a Catch-All Service that will handle everything else

  master.setIgnoreInactiveNodes(true);                                                                                // If a Node doesnt Answer, he will be ignored for now on, till some time
  master.blockingMode(gnMsup1::NearlyAsynchronous);                                                                   // Choose a Blocking-Mode

  master.begin(115200);                                                                                               // Start the MSUP Slave
}



void loop() {
  master.handleCommunication();                                                                                       // Call the MSUP handleCommunication as much as possible; Avoid long delay() Calls!

  if (millis() - delayTimer > 5000) {                                                                                 // Aequivalent to a delay(5000), but non-blocking!
    delayTimer = millis();                                                                                            // Usualy you like to use a much lower value like 10ms or 100ms to be more reactive
    master.pollRange(0x00, 0x0F, 2, true, true);                                                                      // Poll a Range of Slaves for 2 Messages (allow them to send there Push-Messages, with CommitReceive)
                                                                                                                      // Note: by using setIgnoreInactiveNodes(true) and commitReceive Flag, the Master marks nodes with no response and 'blacklist' them.
                                                                                                                      // So next time this nodes will be skiped. Archive more robustness by set retryOnCrFailure (so a failed node will tested two-times bevore blacklisting.

    gnMsup1::comError_t lastComError = master.getLastComError();                                                      // Read out the last Communication-Error and provide some useful informations
    if (lastComError.comErrorCode != gnMsup1::None) {
      devSerialDebug.println();
      devSerialDebug.print(F("AdvMaster:\tCommunication Error occured during last Period: "));
      switch (lastComError.comErrorCode) {
        case gnMsup1::Err_CRInvalid:
          devSerialDebug.println(F("CommitReturn received, but invalid Data."));
          devSerialDebug.println(F("AdvMaster:\tCheck cabling, radio interferences, nodes with wrong firmware, ..."));
          break;
          
        case gnMsup1::Err_CRTimeout:
          devSerialDebug.println(F("CommitReturn run into Timeout."));
          devSerialDebug.println(F("AdvMaster:\tIs there there such a Node in the Network? Check power, cabling, firmware, ..."));
          break;
        
        default:
          devSerialDebug.println(F("AdvMaster:\tUnexcepted!"));
          break;
      }
      devSerialDebug.print(F("AdvMaster:\tSlave Address: 0x"));
      debugPrintHex(lastComError.address);
    }

    devSerialDebug.println();
    devSerialDebug.println(F("AdvMaster:\t#####################################################################################"));
    devSerialDebug.println();
  }
}



// Sample Service-Handler
void handleLed(uint8_t subserviceNumber, uint8_t payload[], uint8_t payloadCount) {
  devSerialDebug.println(F("AdvMaster:\tThis is LED Service Callback on Master"));                                    // Write some Debug-Output (you can use the same Interface for your own Output)

  if (rgbValue[subserviceNumber] < 128) {                                                                             // Toggle the RGB-LED Value based on Subservice
    rgbValue[subserviceNumber] = 128;
  } else if (rgbValue[subserviceNumber] < 255) {
    rgbValue[subserviceNumber] = 255;
  } else {
    rgbValue[subserviceNumber] = 0;
  }
  
  for (uint8_t i = 0; i < 3; i++) {
    if (master.send(i, 0x10, 0x00, rgbValue, 3, false, true)) {                                                       // See the active CommitReceivedFlag. This gives us a Feedback if the Slave has correctly received the Frame
                                                                                                                      // Note: you can set the Push-Flag here if you directly expect a answer from a slave
                                                                                                                      // Avoid this, if the slave needs some time to answer (reading a sensor, ...) and poll the slave later by timer!
      devSerialDebug.print(F("AdvMaster:\tMessage successfully sended to address: 0x"));
      debugPrintHex(i);
      devSerialDebug.println();
    } else {
      devSerialDebug.print(F("AdvMaster:\tError while sending message to address: 0x"));
      debugPrintHex(i);
      devSerialDebug.println();
    }
  }  
}



// Sample CatchAll-Handler
void handleAll(uint8_t serviceNumber, uint8_t subserviceNumber, uint8_t payload[], uint8_t payloadCount) {
  devSerialDebug.println(F("AdvMaster:\tThis is UNIVERSAL (CatchAll) Service Callback on Master"));                   // Write some Debug-Output (you can use the same Interface for your own Output)
  // Here you have to deal with everything, including unknown-Services.
  // For example you can implement a routine that publish a MQTT Message or calls a REST API...
  // Like this you delegate some computing-power to a powerful PC instead of a micro.
}



// DebugPrintHex -> Print a formatted hex-number (0x01 instead of 0x1)
void debugPrintHex(uint8_t value) {
  if (value < 0x10) {
    devSerialDebug.print(0);
  }
  devSerialDebug.print(value, HEX);
}
