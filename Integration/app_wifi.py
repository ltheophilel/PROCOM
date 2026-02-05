import struct
import sys
# import pyautogui
from flask import Flask, request, jsonify, render_template
import socket
import threading
import queue
import time
import cv2
import logging
import numpy as np
from enum import Enum
import webbrowser
import base64
import select
import pygetwindow as gw



all_ip = ["192.168.31.233", "172.20.10.2", "192.168.1.114"]

PICO_IP = all_ip[2]
PICO_PORT = 4242

WIDTH = 80
HEIGHT = 60
IMAGE_SIZE = WIDTH * HEIGHT  # 4800 octets
BUF_SIZE = 1400 # 1024


sock = None
sock_lock = threading.Lock()
running = True

# file de réception non bloquante
rx_queue = queue.Queue()
len_rx = 0
data = bytearray()

app = Flask(__name__)
general_msg_for_debug = ""
p = 0.0
m = 0.0

# ===================== TCP thread ======================
# Désactiver les logs de Werkzeug
global log
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)  # Ne montre que les erreurs

class DATA_TYPE(Enum):
    START_IMG = 0
    IMG = 1
    END_IMG = 2
    MOT_0 = 3
    MOT_1 = 4
    GENERAL = 5
    ALL_IMG = 6
    ALL_IN_ONE = 7

class LEN_DATA(Enum):
    LEN_HEADER = 3
    LEN_GENERAL_MSG = 20
    LEN_MOT = 2
    LEN_FLOAT = 8



def receive_all_in_one(packet_data):
    global data, rx_queue, p, m
    offset = 0
    # print(packet_data)
    # General
    # print(packet_data[0:LEN_DATA.LEN_GENERAL_MSG.value]
    #                            .split(b'\x00', 1)[0])
    general_msg_for_debug = f"{packet_data[0:LEN_DATA.LEN_GENERAL_MSG.value]
                               .split(b'\x00', 1)[0]
                               .decode('utf-8', errors='ignore')
                               .strip()}"
    rx_queue.put(general_msg_for_debug) 
    offset += LEN_DATA.LEN_GENERAL_MSG.value
    # print("Received : " + general_msg_for_debug)
    # Mot 0
    mot0_data = struct.unpack('<h', bytes(packet_data[offset:offset+2]))[0]#(packet_data[offset + 1] << 8) | packet_data[offset]
    offset += LEN_DATA.LEN_MOT.value
    rx_queue.put(f"M0 : {mot0_data}")
    # Mot 1
    mot1_data = struct.unpack('<h', bytes(packet_data[offset:offset+2]))[0]#(packet_data[offset + 1] << 8) | packet_data[offset]
    offset += LEN_DATA.LEN_MOT.value
    rx_queue.put(f"M1 : {mot1_data}")
    # p et m, équation de la droite mx+p
    p = float(packet_data[offset:offset+LEN_DATA.LEN_FLOAT.value-1].decode('utf-8', errors='ignore').strip())
    offset += LEN_DATA.LEN_FLOAT.value
    m = float(packet_data[offset:offset+LEN_DATA.LEN_FLOAT.value-1].decode('utf-8', errors='ignore').strip())
    offset += LEN_DATA.LEN_FLOAT.value
    # print(f"p: {p}, m: {m}")
    # Image
    data = bytearray()  # Réinitialiser les données pour une nouvelle image
    # print([format(b, '08b') for b in packet_data[offset:]])
    for index in range(offset, len(packet_data)):
        huit_pixels = packet_data[index]
        for i in range(8):  # Traiter chaque bit
            pixel_value = 255 if (huit_pixels & (1 << (7 - i))) else 0
            data.append(pixel_value)
    # color = 1  # Méthode alternative (RLE)
    # for index in range(offset, len(packet_data), 2):
    #     nb_pixels = (packet_data[index] << 8) | packet_data[index + 1]
    #     new_data = bytearray([0 if color == 0 else 255] * nb_pixels)
    #     data.extend(new_data)
    #     color = 1 - color  # Alterner entre noir et blanc


def recv_exact(sock, n):
    """ Force la lecture de exactement n octets """
    buffer = bytearray()
    while len(buffer) < n:
        ready = select.select([sock], [], [], 2)
        if ready[0]:
            packet = sock.recv(n - len(buffer))
            if not packet:
                return None
            buffer.extend(packet)
        return buffer

def receive_data(sock):
    global rx_queue, data
    # 1. Lire le header (3 octets) de façon robuste
    header = recv_exact(sock, LEN_DATA.LEN_HEADER.value)
    if not header:
        print("[TCP] Connexion fermée par le serveur.")
        close_everything()
        # raise ConnectionError("Connexion fermée")
    packet_type = header[0]
    packet_size = (header[1] << 8) | header[2]
    
    # 2. Lire le contenu exact du paquet
    packet_data = recv_exact(sock, packet_size)
    if packet_data is None:
        raise ConnectionError("Erreur lors de la réception des données")

    # 3. Traiter le paquet 
    if packet_type == DATA_TYPE.START_IMG.value:
        data = bytearray(packet_data)
    elif packet_type == DATA_TYPE.IMG.value:
        data.extend(packet_data)
    elif packet_type == DATA_TYPE.END_IMG.value:
        data.extend(packet_data)
    elif packet_type == DATA_TYPE.ALL_IMG.value:
        data = bytearray(packet_data)
    elif packet_type == DATA_TYPE.ALL_IN_ONE.value:
        receive_all_in_one(packet_data)
    elif packet_type == DATA_TYPE.MOT_0.value:
        rx_queue.put(f"M0 : {packet_data.decode('utf-8', errors='ignore').strip()}")
    elif packet_type == DATA_TYPE.MOT_1.value:
        rx_queue.put(f"M1 : {packet_data.decode('utf-8', errors='ignore').strip()}")
    else:
        rx_queue.put(f"{packet_data.decode('utf-8', errors='ignore').strip()}")

    return bytes(data), packet_type



def decode_bw_image(raw_bytes, width=WIDTH, height=HEIGHT):
    arr = np.frombuffer(raw_bytes, dtype=np.uint8)
    img = arr.reshape((height, width))  # format H×W
    return img


def draw_line_on_image(img, slope, x_intercept, color=(0, 0, 255), thickness=1):
    """
    Trace une ligne rouge sur une image en niveaux de gris.
    :param img: Image en niveaux de gris (2D numpy array).
    :param slope: Pente de la ligne.
    :param y_intercept: Ordonnée à l'origine (en bas au centre).
    :param color: Couleur de la ligne (BGR, rouge par défaut).
    :param thickness: Épaisseur de la ligne.
    :return: Image BGR avec la ligne tracée.
    """
    # Convertir l'image en BGR (3 canaux)
    img_bgr = cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)

    # Coordonnées de l'image (origine en bas à gauche)
    height, width = img.shape
    y_center = height-1
    x_center = width // 2

    # Ajustement de l'ordonnée à l'origine (origine en bas au centre)
    # adjusted_y_intercept = y_center - y_intercept

    # Calculer les points de la ligne
    y = np.arange(0, height)
    x = slope * (height - 1 - y) + x_intercept

    # Filtrer les points dans les limites de l'image
    valid_points = (y >= 0) & (y < height) & (x >= -width//2) & (x < width//2)
    x_valid = x[valid_points] + x_center
    y_valid = y[valid_points].astype(int)

    # Tracer la ligne rouge (BGR : (0, 0, 255))
    for i in range(len(x_valid) - 1):
        pt1 = (int(x_valid[i]), int(y_valid[i]))
        pt2 = (int(x_valid[i + 1]), int(y_valid[i + 1]))
        cv2.line(img_bgr, pt1, pt2, color, thickness)

    return img_bgr




def tcp_thread():
    global sock, running, temps_debut, log, p, m
    print("[TCP] Thread démarré.")
    while running:
        try:
            temps_debut = time.time()
            raw, packet_type = receive_data(sock)
            if packet_type == DATA_TYPE.END_IMG.value or packet_type == DATA_TYPE.ALL_IN_ONE.value:
                if len(raw) != IMAGE_SIZE:
                    raw = raw + bytes([128] * (IMAGE_SIZE - len(raw)))
                # print(f"[TCP] Image reçue ({len(raw)} octets) en {time.time() - temps_debut:.2f} secondes.")
                img = decode_bw_image(raw)
                img = draw_line_on_image(img, m, p)
                _, buffer = cv2.imencode('.png', img)
                img_base64 = base64.b64encode(buffer).decode('utf-8')
                rx_queue.put(f"IMG_DATA:{img_base64}")
                # cv2.imwrite("static/images/cam.png", img)
            elif packet_type in (DATA_TYPE.MOT_0.value, DATA_TYPE.MOT_1.value, DATA_TYPE.GENERAL.value):
                pass

        except Exception as e:
            print("[TCP] Erreur:", e)
            time.sleep(1)

    print("[TCP] Thread terminé.")

def close_everything():
    global sock
    if sock:
        sock.close()
    sys.exit(0)
    

# ====================== routes Flask ======================
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
    # Si la queue est vide, on renvoie une liste vide au lieu de None
    if rx_queue.empty():
        return jsonify({"data": [], "v_mot0": None, "v_mot1": None})
    
    all_msgs = {"data": [], "v_mot0": 0, "v_mot1": 0}
    while not rx_queue.empty():
        msg = rx_queue.get()
        if msg.startswith("M0 :"): 
            all_msgs["v_mot0"] = msg.split(" : ")[1]
        elif msg.startswith("M1 :"):
            all_msgs["v_mot1"] = msg.split(" : ")[1]
        else:
            all_msgs["data"].append(msg)
    return jsonify(all_msgs)


# ====================== main ======================
if __name__ == "__main__":
    # thread TCP
    log.disabled = False
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setblocking(0)
    sock.settimeout(5)
    print(f"[TCP] Connexion à {PICO_IP}:{PICO_PORT}…")
    sock.connect((PICO_IP, PICO_PORT))
    sock.settimeout(None)
    print("[TCP] Connecté au Pico.")
    t = threading.Thread(target=tcp_thread, daemon=True)
    t.start()

    # démarrage Flask
    print("[WEB] Démarrage serveur…")
    adresse = "http://127.0.0.1:5000"
    webbrowser.open(adresse)
    app.run(host="0.0.0.0", port=5000, debug=False)
    running = False
    t.join()
