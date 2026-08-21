import RPi.GPIO as GPIO
import time

GPIO.setmode(GPIO.BCM)

PWM_PIN = 17
FORWARD_PIN = 27
BACKWARD_PIN = 22

SWITCH_PIN_MINUS = 25
SWITCH_PIN_PLUS = 24

PWM_FREQ = 1000
STEP_DELAY = 0.5

direction = 1
speed = 0
speed_direction = 1

GPIO.setup(PWM_PIN, GPIO.OUT)
GPIO.setup(FORWARD_PIN, GPIO.OUT)
GPIO.setup(BACKWARD_PIN, GPIO.OUT)

GPIO.setup(SWITCH_PIN_MINUS, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
GPIO.setup(SWITCH_PIN_PLUS, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

pwm = GPIO.PWM(PWM_PIN, PWM_FREQ)
pwm.start(0)

def apply_direction():
    if direction == 1:
        GPIO.output(FORWARD_PIN, 1)
        GPIO.output(BACKWARD_PIN, 0)
        print("Direction: +")
    else:
        GPIO.output(FORWARD_PIN, 0)
        GPIO.output(BACKWARD_PIN, 1)
        print("Direction: -")

def stop_motion():
    GPIO.output(FORWARD_PIN, 0)
    GPIO.output(BACKWARD_PIN, 0)

def go_minus(channel):
    global direction
    direction = -1
    apply_direction()

def go_plus(channel):
    global direction
    direction = 1
    apply_direction()

def set_speed(percent):
    percent = max(0, min(100, percent))
    pwm.ChangeDutyCycle(percent)
    print(f"PWM: {percent}%")

try:
    apply_direction()
    set_speed(speed)

    GPIO.add_event_detect(
        SWITCH_PIN_MINUS,
        GPIO.RISING,
        callback=go_minus,
        bouncetime=200
    )

    GPIO.add_event_detect(
        SWITCH_PIN_PLUS,
        GPIO.RISING,
        callback=go_plus,
        bouncetime=200
    )

    while True:
        set_speed(speed)
        time.sleep(STEP_DELAY)

        if speed >= 100:
            speed = 100
            speed_direction = -1

        elif speed <= 0:
            speed = 0
            speed_direction = 1

        speed += speed_direction

except KeyboardInterrupt:
    print("Stopped by user")

finally:
    set_speed(0)
    stop_motion()
    pwm.stop()
    GPIO.cleanup()