from luma.core.interface.serial import i2c, spi, pcf8574
from luma.core.interface.parallel import bitbang_6800
from luma.core.render import canvas
from luma.oled.device import ssd1306, ssd1309, ssd1325, ssd1331, sh1106, sh1107, ws0010
from pathlib import Path
from PIL import Image
import time
import random
import socket
import cv2


people_in = 0
people_out = 0

def start_socket_server(host, port):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((host, port))
    server_socket.listen(1)
    print(f"Socket server listening on {host}:{port}")

    conn, _ = server_socket.accept()
    print("Connected to C++ program")

    while True:
        data = conn.recv(1024).decode('utf-8')
        if not data:
            break
        parts = data.split(', ')
        global people_in, people_out
        people_in = int(parts[0].split(': ')[1])
        people_out = int(parts[1].split(': ')[1])
        with canvas(device) as draw:
            # Display the image in the top right corner
            draw.bitmap((95, 0), image, fill=1)

            # Display text
            draw.text((10, 0), "TRACKING ", fill=1)
            draw.text((10, 10), "SYSTEM ", fill=1)

            draw.text((10, 20), "No people in: ", fill=1)
            draw.text((10, 30), str(people_in), fill=1)
            draw.text((10, 40), "No people out: ", fill=1)
            draw.text((10, 50), str(people_out), fill=1)

    conn.close()
    server_socket.close()

serial = i2c(port=1, address=0x3C)
device = sh1106(serial)

# Load the image
image_path = '/home/raspberrypi/People_Counting_System/OLED_Displaying/images/ktmt.png'  # Replace with the path to your image
image = Image.open(image_path)

# Resize the image to fit the OLED screen (128x64 for sh1106)
image = image.resize((25, 25))

if __name__ == '__main__':
    start_socket_server('192.168.68.115', 4000)









