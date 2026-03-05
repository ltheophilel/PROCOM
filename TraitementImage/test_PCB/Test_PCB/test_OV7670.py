import sys
import time

import board
import busio
import digitalio

from adafruit_ov7670 import (
    OV7670,
    OV7670_COLOR_YUV,
    OV7670_SIZE_DIV16,
)

with digitalio.DigitalInOut(board.GP15) as shutdown:
    shutdown.switch_to_output(True)
    time.sleep(0.001)
    bus = busio.I2C(scl=board.GP27, sda=board.GP26)
cam = OV7670(
    bus,
    data_pins=[
        board.GP11,
        board.GP12,
        board.GP13,
        board.GP14,
        board.GP15,
        board.GP16,
        board.GP17,
        board.GP18,
    ],
    clock=board.GP21,
    vsync=board.GP22,
    href=board.GP9,
    mclk=board.GP10,
    shutdown=board.GP19,
    reset=board.GP20,
)
cam.size = OV7670_SIZE_DIV16
cam.colorspace = OV7670_COLOR_YUV
cam.flip_y = True

buf = bytearray(2 * cam.width * cam.height)
chars = b" .:-=+*#%@"

width = cam.width
row = bytearray(2 * width)

sys.stdout.write("\033[2J")
while True:
    cam.capture(buf)
    for j in range(cam.height):
        sys.stdout.write(f"\033[{j}H")
        for i in range(cam.width):
            row[i * 2] = row[i * 2 + 1] = chars[
                buf[2 * (width * j + i)] * (len(chars) - 1) // 255
            ]
        sys.stdout.write(row)
        sys.stdout.write("\033[K")
    sys.stdout.write("\033[J")
    time.sleep(1)
