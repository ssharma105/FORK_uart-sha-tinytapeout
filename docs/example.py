import serial, binascii

# Set to your TTY device.
ser = serial.Serial("/dev/ttyUSB0", 9600)

# Send 64 Bytes of input. Hashing "abc" with specified padding format.
print("Writing")
ser.write(b'abc\x80')
for i in range(0,59):
    ser.write(b'\x00')
ser.write(b'\x18')


# Read 20 Bytes of output.
print("Reading")
print( binascii.hexlify( ser.read(20) ) )
