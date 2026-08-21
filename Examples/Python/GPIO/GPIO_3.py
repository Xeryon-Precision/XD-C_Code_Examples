# 1.  Connect pins 19, 23, 25, 27 and 30 with the pins you want to use on the Raspberry Pi.
# 2.  Connect the XD-C to a computer with an USB C cable.
# 3.  Power the XD-C.
# 4.  Open a program that you can use to send commands over a COM port.
# 5.  Connect with the right port on baudrate 115200.
# 6.  If you do not want all the info updates, send "INFO=0".
# 7.  Send the command "GPIO=3".
# 8.  If you want to check, you can type "GPIO=?", and it will show the value of GPIO.
# 9.  Standard, STPS is equal to 1. I changed it to 3200. So type "STPS=3200". If you want to check the value, type "STPS=?"
# 10. If you want to save this parameters, send "SAVE". If you power off the XD-C without the "SAVE", all the paremeters are reset.
# 11. Open the Python code, and change the pins to the pins you want.
# 12. Run the script

import RPi.GPIO as GPIO
import time

GPIO.setmode(GPIO.BCM)

PULSE = 17
FORWARD = 27
BACKWARD = 22
INDEX = 23

GPIO.setup(PULSE, GPIO.OUT)
GPIO.setup(FORWARD, GPIO.OUT)
GPIO.setup(BACKWARD, GPIO.OUT)
GPIO.setup(INDEX, GPIO.OUT)

# ---------- FUNCTIONS ----------
def pulse(width=0.001):
    GPIO.output(PULSE, 1)
    time.sleep(width)
    GPIO.output(PULSE, 0)

def forward(steps):
    GPIO.output(FORWARD, 1)
    GPIO.output(BACKWARD, 0)

    for _ in range(steps):
        pulse()
        time.sleep(0.01)

def backward(steps):
    GPIO.output(FORWARD, 0)
    GPIO.output(BACKWARD, 1)

    for _ in range(steps):
        pulse()

def index():
    GPIO.output(INDEX, 0)
    time.sleep(1)
    GPIO.output(INDEX, 1)
    time.sleep(1)
    GPIO.output(INDEX, 0)

# ---------- TEST ----------
try:
    index()
    time.sleep(2)
    while True:
        for _ in range(0, 10):
            forward(1)
            time.sleep(0.5)
        for _ in range(0, 20):    
            backward(1)
            time.sleep(0.05)
        for _ in range(0, 10):
            forward(1)
            time.sleep(0.5)

finally:
    GPIO.cleanup()