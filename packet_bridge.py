import serial
import struct

magic_prefix = b"ZPB";

port = serial.Serial(port = "/dev/ttyACM0",
        baudrate = 115200,
        bytesize = 8,
        parity = serial.PARITY_NONE,
        stopbits = serial.STOPBITS_ONE,
        xonxoff = False,
        rtscts = False,
        timeout = 0.1)

port.flushInput()
port.flushOutput()

def write_packet(command, request_id, data):
    packet = magic_prefix + struct.pack("!BHH", command, request_id, len(data)) + data;
    port.write(packet)

write_packet(123, 456, b"Hello world")

data = port.read(len(magic_prefix)+5)
if data[0:len(magic_prefix)] != magic_prefix:
    print("Magic prefix does not match")
command, request_id, data_len = struct.unpack("!BHH", data[len(magic_prefix):])
print("Command:", command)
print("Request ID:", request_id)
data = port.read(data_len)
print("Data", data)

