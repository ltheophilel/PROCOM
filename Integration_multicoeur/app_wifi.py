from flask import Flask, request, jsonify, render_template
import socket
import threading
import queue
import time
import cv2
import logging
import numpy as np
from enum import Enum

PICO_IP = "10.244.156.53"
PICO_PORT = 4242

WIDTH = 80
HEIGHT = 60
IMAGE_SIZE = WIDTH * HEIGHT  # 4800 octets
BUF_SIZE = 1024

sock = None
sock_lock = threading.Lock()
running = True

# file de réception non bloquante
rx_queue = queue.Queue()
data = bytearray()

app = Flask(__name__)

# Désactiver les logs de Werkzeug
log = logging.getLogger('werkzeug')


class DATA_TYPE(Enum):
    START_IMG = 0
    IMG = 1
    END_IMG = 2
    MOT_0 = 3
    MOT_1 = 4
    GENERAL = 5


def recv_exact(sock, n):
    """Lit exactement n octets depuis le socket"""
    buf = bytearray()
    while len(buf) < n:
        part = sock.recv(n - len(buf))
        if not part:
            raise ConnectionError("Connexion fermée pendant recv_exact")
        buf.extend(part)
    return bytes(buf)


def receive_image(sock):
    """
    Lit une image chunkée depuis le Pico en TCP, en reconstruisant correctement
    START_IMG / IMG / END_IMG. Retourne les données complètes et le type du dernier paquet.
    """
    data = bytearray()

    while True:
        header = recv_exact(sock, 3)
        packet_type = header[0]
        packet_size = (header[1] << 8) | header[2]

        chunk = recv_exact(sock, packet_size)
        data.extend(chunk)

        if packet_type == DATA_TYPE.END_IMG.value:
            return bytes(data), DATA_TYPE.END_IMG.value
        elif packet_type in (DATA_TYPE.MOT_0.value, DATA_TYPE.MOT_1.value, DATA_TYPE.GENERAL.value):
            return chunk, packet_type


def decode_bw_image(raw_bytes, width=WIDTH, height=HEIGHT):
    arr = np.frombuffer(raw_bytes, dtype=np.uint8)
    img = arr.reshape((height, width))  # format H×W
    return img


def tcp_thread():
    global sock, running
    print("[TCP] Thread démarré.")
    while running:
        try:
            raw, packet_type = receive_image(sock)
            if packet_type == DATA_TYPE.END_IMG.value:
                if len(raw) != IMAGE_SIZE:
                    print(f"[TCP] Taille d'image incorrecte : {len(raw)} octets reçus au lieu de {IMAGE_SIZE}.")
                    continue
                img = decode_bw_image(raw)
                cv2.imwrite("static/images/cam.png", img)
            elif packet_type in (DATA_TYPE.MOT_0.value, DATA_TYPE.MOT_1.value, DATA_TYPE.GENERAL.value):
                rx_queue.put(raw.decode("utf-8", errors='ignore').strip())
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
    all_msgs = {}
    msgs = []
    start_time = time.time()
    while not rx_queue.empty() and time.time() - start_time < 2:
        msg = rx_queue.get()
        if msg.startswith("M0 :"):
            all_msgs["v_mot0"] = msg.split("M0 :")[1].strip()
        elif msg.startswith("M1 :"):
            all_msgs["v_mot1"] = msg.split("M1 :")[1].strip()
        else:
            msgs.append(msg)
    all_msgs["data"] = msgs if msgs else None
    return jsonify(all_msgs)


if __name__ == "__main__":
    # thread TCP
    log.disabled = False
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(5)
    print(f"[TCP] Connexion à {PICO_IP}:{PICO_PORT}…")
    sock.connect((PICO_IP, PICO_PORT))
    sock.settimeout(None)
    print("[TCP] Connecté au Pico.")
    t = threading.Thread(target=tcp_thread, daemon=True)
    t.start()

    # démarrage Flask
    print("[WEB] Démarrage serveur…")
    app.run(host="0.0.0.0", port=5000, debug=False)
    running = False
    t.join()
