import RPi.GPIO as GPIO
import time

# Define GPIO pins
button_pin = 26
buzzer_pin = 13

# Setup
GPIO.setmode(GPIO.BCM)
GPIO.setup(button_pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(buzzer_pin, GPIO.OUT)

print("Press the button to activate the buzzer...")

try:
    button_pressed = False  # Track the state of the button

    while True:
        if GPIO.input(button_pin) == GPIO.LOW:  # Button is pressed
            if not button_pressed:  # Only act if the button state has changed
                GPIO.output(buzzer_pin, GPIO.HIGH)  # Turn buzzer ON
                print("Button Pressed - Buzzer ON")
                button_pressed = True
        else:  # Button is released
            if button_pressed:  # Only act if the button state has changed
                GPIO.output(buzzer_pin, GPIO.LOW)  # Turn buzzer OFF
                print("Button Released - Buzzer OFF")
                button_pressed = False

        time.sleep(0.1)  # Short delay to debounce the button
except KeyboardInterrupt:
    print("Exiting...")
finally:
    GPIO.cleanup()
