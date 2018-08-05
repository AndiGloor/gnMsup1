# gnMsup1
Arduino library for generic master/slave communications.
Currently only RS485 as a hardare-layer is implemented. But its easy to implement additional layers like RS232 or others. You can use a shared medium or point2point communication.
Its a message-based protocol, sending frames between master and slave.

# Roles
## Master
Each environment requires one master. It controls the bus and request its slaves to send data. A master can talk to everyone in the environment.

## Slave
A environment can have one or many slaves. Each slave is independent, but requires a master. A slave is just able to talk with the master.

# Protocol Structure
## Frame
| Byte | Type/Value | Description |
| --- | --- | --- |
| 00<br/>01 | `0xAA`<br/>`0x55` | __StartBytes__<br/>`1010 1010` // `0101 0101` |
| 02 | Flagbyte | See [below](#flagbyte) |

<a name="flagbyte" />
## Flagbyte
ABC

# License
GnMsup1 stands under the MIT License.

Copyright (c) 2018 Andreas Gloor

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
