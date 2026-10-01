import snap7
ptc=snap7.client.Client()
plc.connect("192.168.1.10",0.1)
data=plc.db_read(3, 0, 10)
print(data)
plc.disconnect()
