from flask import Flask, request, jsonify, render_template
import socket
import threading
import queue
import time
import cv2
import logging
import numpy as np
from enum import Enum


PICO_IP = "172.20.10.2"
PICO_PORT = 4242

WIDTH = 80
HEIGHT = 60
IMAGE_SIZE = WIDTH * HEIGHT  # 4800 octets
BUF_SIZE = 1024
HEADER_SIZE = 3

sock = None
sock_lock = threading.Lock()
running = True

# file de réception non bloquante
rx_queue = queue.Queue()
data = bytearray()

app = Flask(__name__)

# Désactiver les logs de Werkzeug
global log
log = logging.getLogger('werkzeug')
# log.setLevel(logging.ERROR)  # Ne montre que les erreurs

class DATA_TYPE(Enum):
    START_IMG = 0
    IMG = 1
    END_IMG = 2
    MOT_0 = 3
    MOT_1 = 4
    GENERAL = 5


def receive_image(sock):
    global temps_debut, rx_queue, data
    while True:
        # print(f"[TCP] Réception des données… {len(data)}/{IMAGE_SIZE} octets reçus.")
        header = sock.recv(3)
        if not header:
            raise ConnectionError("Connexion fermée")
        
        # Extraire le type et la taille du paquet
        packet_type = header[0]
        packet_size = (header[1] << 8) | header[2]  # Taille sur 2 octets (big-endian)
        
        # Lire les données du paquet
        packet_data = sock.recv(packet_size)
        if not packet_data or len(packet_data) != packet_size:
            print(f"[TCP] Erreur de réception : attendu {packet_size} octets, reçu {len(packet_data) if packet_data else 0} octets.")
            raise ConnectionError("Erreur lors de la réception des données")
        # print(f"[TCP] Reçu {len(packet_data)} octets (type: {packet_type}, taille: {packet_size})")
        # print(packet_type)
        # Traiter le paquet en fonction de son type
        if packet_type == DATA_TYPE.START_IMG.value:
            data = bytearray()  # Réinitialiser les données pour une nouvelle image
            data.extend(packet_data)
        elif packet_type == DATA_TYPE.END_IMG.value:
            data.extend(packet_data)
            break  # Fin de l'image
        elif packet_type == DATA_TYPE.IMG.value:
            data.extend(packet_data)
        elif packet_type == DATA_TYPE.MOT_0.value:
            rx_queue.put(f"M0 : {packet_data.decode('utf-8', errors='ignore').strip()}")
            break
        elif packet_type == DATA_TYPE.MOT_1.value:
            rx_queue.put(f"M1 : {packet_data.decode('utf-8', errors='ignore').strip()}")
            break
        else:
            rx_queue.put(f"{packet_data.decode('utf-8', errors='ignore').strip()}")
            break
    return bytes(data), packet_type


def decode_bw_image(raw_bytes, width=80, height=60):
    arr = np.frombuffer(raw_bytes, dtype=np.uint8)
    img = arr.reshape((height, width))  # format H×W
    return img

def tcp_thread():
    global sock, running, temps_debut, log
    print("[TCP] Thread démarré.")
    while running:
        try:
            temps_debut = time.time()
            raw, packet_type = receive_image(sock)
            if packet_type == DATA_TYPE.END_IMG.value:
                if len(raw) != IMAGE_SIZE:
                    print(f"[TCP] Taille d'image incorrecte : {len(raw)} octets reçus au lieu de {IMAGE_SIZE}.")
                    continue
                img = decode_bw_image(raw)
                cv2.imwrite("static/images/cam.png", img)
                # print(f"[TCP] Image reçue et sauvegardée en {time.time() - temps_debut:.2f} secondes.")
            elif packet_type in (DATA_TYPE.MOT_0.value, DATA_TYPE.MOT_1.value, DATA_TYPE.GENERAL.value):
                pass
            # data = sock.recv(BUF_SIZE)
            # text = data.decode("utf-8", errors="ignore").strip()
            # rx_queue.put(text)

        except Exception as e:
            print("[TCP] Erreur:", e)
            time.sleep(1)

    print("[TCP] Thread terminé.")


@app.route("/")
def index():
    return render_template("index.html")


@app.route("/send", methods=["POST"])
def send_cmd():
    cmd = request.json.get("command", "")
    print("[WEB] Envoi commande :", cmd)

    with sock_lock:
        sock.send((cmd + "\n").encode("utf-8"))

    return jsonify({"status": "ok"})


@app.route("/read")
def read():
    if not log.disabled:
        log.disabled = True
    if not rx_queue.empty():
        msgs = []
        all_msgs = {}
        temps_debut = time.time()
        while not rx_queue.empty() and time.time() - temps_debut < 2:
            msg = rx_queue.get()
            if msg.startswith("M0 :"): 
                v_mot = msg.split("M0 :")[1].strip()
                all_msgs["v_mot0"] = v_mot
            elif msg.startswith("M1 :"):
                v_mot = msg.split("M1 :")[1].strip()
                all_msgs["v_mot1"] = v_mot
            else:
                print("[WEB] Message reçu :", msg)
                msgs.append(msg)
                all_msgs["data"] = msgs
        return jsonify(all_msgs)
    return jsonify({"data": None})



if __name__ == "__main__":
    # thread TCP
    log.disabled = False
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(5)
    print(f"[TCP] Connexion à {PICO_IP}:{PICO_PORT}…")
    sock.connect((PICO_IP, PICO_PORT))
    sock.settimeout(None)
    print("[TCP] Connecté au Pico.")
    t = threading.Thread(target=tcp_thread)
    t.start()

    # démarrage Flask
    print("[WEB] Démarrage serveur…")
    app.run(host="0.0.0.0", port=5000, debug=False)
    running = False
    t.join()
