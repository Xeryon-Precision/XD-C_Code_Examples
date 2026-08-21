import RPi.GPIO as GPIO
import time

GPIO.setmode(GPIO.BCM)

# Raspberry Pi GPIO pins
PULSE = 17
DIRECTION = 27
ENABLE = 22
INDEX = 23

GPIO.setup(PULSE, GPIO.OUT)
GPIO.setup(DIRECTION, GPIO.OUT)
GPIO.setup(ENABLE, GPIO.OUT)
GPIO.setup(INDEX, GPIO.OUT)

# ---------- FUNCTIONS ----------
def pulse(width=0.001):
    GPIO.output(PULSE, 1)
    time.sleep(width)
    GPIO.output(PULSE, 0)

def enable():
    GPIO.output(ENABLE, 1)

def disable():
    GPIO.output(ENABLE, 0)

def forward(steps):
    GPIO.output(DIRECTION, 1)

    for _ in range(steps):
        pulse()
        time.sleep(0.01)

def backward(steps):
    GPIO.output(DIRECTION, 0)

    for _ in range(steps):
        pulse()
        time.sleep(0.01)

def index():
    GPIO.output(INDEX, 0)
    time.sleep(1)
    GPIO.output(INDEX, 1)
    time.sleep(1)
    GPIO.output(INDEX, 0)

# ---------- TEST ----------
try:
    enable()

    index()
    time.sleep(2)

    while True:
        for _ in range(10):
            forward(1)
            time.sleep(0.5)

        for _ in range(20):
            backward(1)
            time.sleep(0.05)

        for _ in range(10):
            forward(1)
            time.sleep(0.5)

finally:
    disable()
    GPIO.cleanup()