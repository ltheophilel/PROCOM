import serial
import numpy as np
import cv2
import time

# === CONFIGURATION ===
PORT = "/dev/ttyACM0"
BAUD = 115200      # Le baud est ignoré par USB CDC mais requis par pyserial
TIMEOUT = 1
SCREEN_RES = 1920, 1080 # Resolution de l'ecran

# Ouvre le port série
ser = serial.Serial(PORT, BAUD, timeout=TIMEOUT)

# Fenêtre OpenCV redimensionnable
cv2.namedWindow("OV7670 Video", cv2.WINDOW_NORMAL)
cv2.setWindowProperty("OV7670 Video", cv2.WND_PROP_FULLSCREEN, cv2.WINDOW_FULLSCREEN)

# Mesure de FPS en transmission
frame = 0
start = time.time()

while True:
    try:
        # Lire l'en-tête PPM/PGM
        header = ser.readline().decode(errors='ignore').strip()
        if header not in ("P6", "P5"):
            continue

        # Lecture des dimensions
        dims = ser.readline().decode(errors='ignore').strip()
        if " " not in dims:
            continue
        width, height = map(int, dims.split())

        # Lire la valeur max
        maxval = ser.readline().decode(errors='ignore').strip()
        if maxval != "255":
            continue

        # Déterminer la taille des données
        if header == "P6":
            expected_bytes = width * height * 3     # RGB
        else:  # P5
            expected_bytes = width * height         # Echelle de gris (Y seul)

        # Lecture de l'image
        data = bytearray()
        while len(data) < expected_bytes:
            chunk = ser.read(expected_bytes - len(data))
            if not chunk:
                break
            data.extend(chunk)
        if len(data) != expected_bytes:
            continue

        # Conversion en image numpy
        if header == "P6":
            # RGB
            img = np.frombuffer(data, dtype=np.uint8).reshape((height, width, 3))
        else:
            # Echelle de gris
            gray = np.frombuffer(data, dtype=np.uint8).reshape((height, width))
            img = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR)

        # Afficher l'image
        resized_img = cv2.resize(img, SCREEN_RES, interpolation=cv2.INTER_NEAREST)
        cv2.imshow("OV7670 Video", resized_img)
        frame+=1
        # Quitter avec Echap
        if cv2.waitKey(1) & 0xFF == 27:
            end = time.time()
            break

    except KeyboardInterrupt:
        break

cv2.destroyAllWindows()
ser.close()
delta_t = end-start
fps = frame/delta_t
print(fps) # En transmission bien sur
