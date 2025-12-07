#!/usr/bin/env python3
"""
ascii_dump.py

Lit le flux série du Pico octet par octet et affiche
les caractères ASCII dans le terminal. Les octets non
imprimables sont remplacés par '.'
"""

import serial
import sys

# --- CONFIGURATION ---
PORT = "/dev/ttyACM0"  # remplace si nécessaire
BAUD = 115200           # vitesse série
TIMEOUT = 0.1           # en secondes

# --- OUVERTURE DU PORT ---
try:
    ser = serial.Serial(PORT, BAUD, timeout=TIMEOUT)
    print(f"Connecté à {PORT} à {BAUD} bps")
except Exception as e:
    print(f"Erreur ouverture port {PORT} :", e)
    sys.exit(1)

print("Lecture... Ctrl+C pour arrêter\n")

try:
    while True:
        byte = ser.read(1)  # lit 1 octet
        if not byte:
            continue
        # convertir en ASCII si imprimable, sinon '.'
        b = byte[0]
        if 32 <= b <= 126:  # ASCII imprimable
            print(chr(b), end='', flush=True)
        elif b == 10:  # saut de ligne
            print('', flush=True)
        else:
            print('.', end='', flush=True)

except KeyboardInterrupt:
    print("\nArrêt demandé (Ctrl+C)")

finally:
    ser.close()
