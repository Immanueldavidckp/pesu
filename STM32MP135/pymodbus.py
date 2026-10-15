from pymodbus.client import ModbusSerialClient
 
client = ModbusSerialClient(port="/dev/ttyUSB0", baudrate=9600, parity="N",
                            stopbits=1, bytesize=8, timeout=1)   # Create client object
client.connect()
 
client.write_coils(address=0, values=[0,1,0,1,0,1,0,1,0,1,0,1], device_id=1)
 
result = client.read_coils(address=0, count=16, device_id=1)
