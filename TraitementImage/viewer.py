import serial
import numpy as np
import cv2

# === CONFIGURATION ===
PORT = "COM4"
BAUD = 115200      # Le baud est ignoré par USB CDC mais requis par pyserial
TIMEOUT = 1

# Ouvre le port série
ser = serial.Serial(PORT, BAUD, timeout=TIMEOUT)

# Fenêtre OpenCV redimensionnable
cv2.namedWindow("OV7670 Video", cv2.WINDOW_NORMAL)
cv2.setWindowProperty("OV7670 Video", cv2.WND_PROP_FULLSCREEN, cv2.WINDOW_FULLSCREEN)

while True:
    try:
        # Lire l'en-tête PPM/PGM
        header = ser.readline().decode(errors='ignore').strip()
        if header not in ("P6", "P5"):
            continue

        # Lire les dimensions
        dims = ser.readline().decode(errors='ignore').strip()
        if " " not in dims:
            continue
        width, height = map(int, dims.split())

        # Lire maxval
        maxval = ser.readline().decode(errors='ignore').strip()
        if maxval != "255":
            continue

        # Déterminer la taille des données
        if header == "P6":
            expected_bytes = width * height * 3     # RGB
        else:  # P5
            expected_bytes = width * height         # grayscale (Y only)

        # Lire l'image
        data = bytearray()
        while len(data) < expected_bytes:
            chunk = ser.read(expected_bytes - len(data))
            if not chunk:
                break
            data.extend(chunk)
        if len(data) != expected_bytes:
            continue

        # Convertir en image numpy
        if header == "P6":
            # RGB
            img = np.frombuffer(data, dtype=np.uint8).reshape((height, width, 3))
        else:
            # Grayscale
            gray = np.frombuffer(data, dtype=np.uint8).reshape((height, width))
            img = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR)

        # Afficher la frame
        cv2.imshow("OV7670 Video", img)

        # Quitter avec ESC
        if cv2.waitKey(1) & 0xFF == 27:
            break

    except KeyboardInterrupt:
        break

cv2.destroyAllWindows()
ser.close()