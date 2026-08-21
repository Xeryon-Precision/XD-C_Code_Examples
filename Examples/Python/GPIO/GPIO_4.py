import RPi.GPIO as GPIO
import time

GPIO.setmode(GPIO.BCM)

A = 17
B = 27
ENABLE = 22
INDEX = 23

GPIO.setup(A, GPIO.OUT)
GPIO.setup(B, GPIO.OUT)
GPIO.setup(ENABLE, GPIO.OUT)
GPIO.setup(INDEX, GPIO.OUT)

quad_states = [
    (0, 0),
    (1, 0),
    (1, 1),
    (0, 1),
]

quad_index = 0

GPIO.output(A, quad_states[quad_index][0])
GPIO.output(B, quad_states[quad_index][1])
GPIO.output(ENABLE, 0)
GPIO.output(INDEX, 0)

def enable():
    GPIO.output(ENABLE, 1)

def disable():
    GPIO.output(ENABLE, 0)

def index():
    GPIO.output(INDEX, 0)
    time.sleep(1)
    GPIO.output(INDEX, 1)
    time.sleep(1)
    GPIO.output(INDEX, 0)

def one_mm_forward(delay=0.001):
    global quad_index

    quad_index = (quad_index + 1) % 4
    GPIO.output(A, quad_states[quad_index][0])
    GPIO.output(B, quad_states[quad_index][1])
    time.sleep(delay)

def one_mm_backward(delay=0.001):
    global quad_index

    quad_index = (quad_index - 1) % 4
    GPIO.output(A, quad_states[quad_index][0])
    GPIO.output(B, quad_states[quad_index][1])
    time.sleep(delay)

def forward(mm):
    for _ in range(mm):
        one_mm_forward()
        time.sleep(0.5)

def backward(mm):
    for _ in range(mm):
        one_mm_backward()
        time.sleep(0.05)

try:
    enable()

    index()
    time.sleep(2)
    while True:
        forward(10)   # normaal nu 1 mm
        backward(20)
        forward(10)

finally:
    disable()
    GPIO.cleanup()