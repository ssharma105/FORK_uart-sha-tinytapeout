## How it works

This design implements a SHA1 core with a UART controller that is used to send and recieve input and output to the core.

The SHA1 core is borrowed from SecWorks [https://github.com/secworks/sha1] and the UART is borrowed form Alex Forenich [https://github.com/alexforencich/verilog-uart] - both Open Source IPs.

## How to test

On the Tiny Tapeout motherboard, the pin connection is as follows:

| Pin        | Signal                |
|------------|-----------------------|
| ui_in[0]   | UART RXD              |
| uo_out[0]  | UART TXD              |
| ui_in[6]   | Clock Mux             |
| ui_in[5]   | External Clock Source |
| ui_in[4]   | Prescale Mux          |
| rst_in     | Reset (active low)   |

### Operation Description


#### Clock 

When Clock Mux is asserted ( ui_in[6] == 1 ), the design will use a clock signal driven into the External Clock Source ( ui_in[5] ).

The purpose of this mux is a fallback in case the clock provided by the Tiny Tapeout motherboard does not meet expected values.

#### UART 

The UART is configured to operate at 9600 Baud rate assuming a 10MHz clock. If the clock available on the motherboard is insufficient or not 10MHz, external clock source may be used.

The Prescale Mux selector can be used to enforce a 1:1 UART Clock. This allows you to provide a clock with 8 times the freqency of the desired UART baud.

Eg) If you want a 115200 baud rate, you can drive a 921.600KHz clock into the external clock source.

#### Interaction with SHA Core

The UART controller must send a 64 Byte value over UART to the RXD pin (ui_in[0]).

This 64 Byte "block" of data should be padded appropriately. The scheme for padding is as follows:

[DATA][0x80][0x00..][LENGTH IN BITS]

For example, to Hash the value "abc", the bytes

`512'h61626380000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000018`

should be sent. Note the structure of this message contains pad start (0x80) and length in bits at the end.

Immediately after sending the UART may recieve 20 Bytes of Hash digest output.

As soon as the 20 Byte response is sent, the circuit becomes available to recieve new input. The entire transaction must be completed before new data can be consumed.

`example.py` in docs folder contains a very basic script demonstrating a transaction.

#### Reset

At any time the circuit may be reset. After reset the circuit may recieve new data and begin a new transaction.

#### Implementation Notes

This circuit cannot create a running digest. It will only digest 512 bit messages. Additional messages sent should be considered atomically. 


## External hardware

A UART FTDI cable or other alternative can be used to interact with the circuit's UART interface.

A function generator can optionally be used to drive the external clock source. 

## Hardware Trojan

We implemented a "hardware trojan" into the design. Can you find it?

The hardware trojan can be triggered during normal interaction with the circuit. 

Occasionally you may recieve the wrong output. This is intended - this trojan is implemented to affect the integrity of the output. 
