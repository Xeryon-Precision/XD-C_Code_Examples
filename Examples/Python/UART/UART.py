import serial
import time

ser = serial.Serial(
    port='/dev/ttyAMA0',
    baudrate=9600, 
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    timeout=1
)

time.sleep(2)

def recvData(cmd):
    ser.reset_input_buffer()
    ser.write((cmd + '\r\n').encode())

    response = ser.readline().decode('ascii', errors='ignore').strip()
    return response

def sendData(cmd):
    ser.write((cmd + '\r\n').encode())
    ser.flush()
    
def get_stat():
    ser.write(b"STAT=?\r\n")
    ser.flush()

    response = ser.readline().decode(errors="ignore").strip()

    if response.startswith("STAT="):
        return int(response.split("=")[1])

    return None

def wait_for_bit8(timeout=10):
    start = time.time()

    while time.time() - start < timeout:
        stat = get_stat()

        if stat is not None:
            if stat & (1 << 8):
                time.sleep(2)
                return True

        time.sleep(0.01)

    return False

# Index
sendData("INDX=1")
if wait_for_bit8():
    print("Index found")
else:
    print("Timeout")
    
# Scan full stroke
sendData("SCAN=-1")
time.sleep(2)
sendData("SCAN=1")
time.sleep(2)

sendData("SSPD=1000")

# Scan with timeout
sendData("SCAN=-1")
time.sleep(2)
sendData("STOP=1")

sendData("SSPD=10000")

# DPOS
sendData("DPOS=0")
time.sleep(2)
sendData("DPOS=10000")
time.sleep(2)
sendData("DPOS=-10000")
time.sleep(2)
sendData("DPOS=0")
time.sleep(2)

# Stepping
for _ in range(0, 10):
    sendData("STEP=3200")
    time.sleep(0.5)


time.sleep(1)
sendData("RSET=1")
time.sleep(1)

ser.close()