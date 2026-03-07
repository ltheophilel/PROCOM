# ===================== TCP thread ======================
# Désactiver les logs de Werkzeug
import queue
import struct
import sys
import time
import cv2
import logging
import numpy as np
from enum import Enum
import base64
import select


running = True

global sock, rx_queue


data = bytearray()

general_msg = ""
p = 0.0
m = 0.0
angle = 0.0
p_aplati = 0.0
m_aplati = 0.0


WIDTH = 80
HEIGHT = 60
IMAGE_SIZE = WIDTH * HEIGHT  # 4800 octets

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
    LEN_GENERAL_MSG = 30
    LEN_MOT = 2
    LEN_FLOAT = 8
    LEN_IMG = 600

def read_general_msg_from_bytes(data, offset):  
    general_msg = f"{data[offset:offset+LEN_DATA.LEN_GENERAL_MSG.value]
                               .split(b'\x00', 1)[0]
                               .decode('utf-8', errors='ignore')
                               .strip()}"
    return general_msg, offset + LEN_DATA.LEN_GENERAL_MSG.value

def read_uint16_from_bytes(data, offset):
     mot0_data = struct.unpack('<h', bytes(data[offset:offset+LEN_DATA.LEN_MOT.value]))[0]
     return mot0_data, offset + LEN_DATA.LEN_MOT.value

def read_float_from_bytes(data, offset):
    float_data = float(data[offset:offset+LEN_DATA.LEN_FLOAT.value-1].decode('utf-8', errors='ignore').strip())
    return float_data, offset + LEN_DATA.LEN_FLOAT.value

def read_image_from_bytes(data, offset):
    img_data = bytearray()  # Réinitialiser les données pour une nouvelle image
    # print([format(b, '08b') for b in packet_data[offset:]])
    for index in range(offset, offset + LEN_DATA.LEN_IMG.value):
        huit_pixels = data[index]
        for i in range(8):  # Traiter chaque bit
            pixel_value = 255 if (huit_pixels & (1 << (7 - i))) else 0
            img_data.append(pixel_value)
    return img_data, offset + LEN_DATA.LEN_IMG.value
    # color = 1  # Méthode alternative (RLE)
    # for index in range(offset, len(packet_data), 2):
    #     nb_pixels = (packet_data[index] << 8) | packet_data[index + 1]
    #     new_data = bytearray([0 if color == 0 else 255] * nb_pixels)
    #     data.extend(new_data)
    #     color = 1 - color  # Alterner entre noir et blanc

def read_from_bytes(data, offset, type):
    if type == "general_msg":
        return read_general_msg_from_bytes(data, offset)
    elif type == "uint16":
        return read_uint16_from_bytes(data, offset)
    elif type == "float":
        return read_float_from_bytes(data, offset)
    elif type == "image":
        return read_image_from_bytes(data, offset)
    else:
        raise ValueError(f"Type de données inconnu : {type}")

def read_all_in_one_from_bytes(data):
    offset = 0
    general_msg, offset = read_general_msg_from_bytes(data, offset)
    mot0_data, offset = read_uint16_from_bytes(data, offset)
    mot1_data, offset = read_uint16_from_bytes(data, offset)
    p, offset = read_float_from_bytes(data, offset)
    m, offset = read_float_from_bytes(data, offset)
    angle, offset = read_float_from_bytes(data, offset)
    p_aplati, offset = read_float_from_bytes(data, offset)
    m_aplati, offset = read_float_from_bytes(data, offset)
    angle_aplati, offset = read_float_from_bytes(data, offset)
    img_data, offset = read_image_from_bytes(data, offset)
    return general_msg, mot0_data, mot1_data, p, m, angle, p_aplati, m_aplati, angle_aplati, img_data

def correct_perspective(image, M):
    # Appliquer la transformation
    corrected = cv2.warpPerspective(image, M, (140, 190))
    return corrected

def receive_all_in_one(packet_data):
    global data, rx_queue, p, m, angle, p_aplati, m_aplati, angle_aplati
    offset = 0
    general_msg, offset = read_general_msg_from_bytes(data, offset)
    mot0_data, offset = read_uint16_from_bytes(data, offset)
    mot1_data, offset = read_uint16_from_bytes(data, offset)
    p, offset = read_float_from_bytes(data, offset)
    m, offset = read_float_from_bytes(data, offset)
    angle, offset = read_float_from_bytes(data, offset)
    p_aplati, offset = read_float_from_bytes(data, offset)
    m_aplati, offset = read_float_from_bytes(data, offset)
    angle_aplati, offset = read_float_from_bytes(data, offset)
    data, offset = read_image_from_bytes(data, offset)
    rx_queue.put(general_msg)
    rx_queue.put(f"M0 : {mot0_data}")
    rx_queue.put(f"M1 : {mot1_data}")


    # print(packet_data)
    # General
    # print(packet_data[0:LEN_DATA.LEN_GENERAL_MSG.value]
    #                            .split(b'\x00', 1)[0])


    # general_msg = f"{packet_data[0:LEN_DATA.LEN_GENERAL_MSG.value]
    #                            .split(b'\x00', 1)[0]
    #                            .decode('utf-8', errors='ignore')
    #                            .strip()}"
    # rx_queue.put(general_msg) 
    # offset += LEN_DATA.LEN_GENERAL_MSG.value
    # # print("Received : " + general_msg)
    # # Mot 0
    # mot0_data = struct.unpack('<h', bytes(packet_data[offset:offset+2]))[0]#(packet_data[offset + 1] << 8) | packet_data[offset]
    # offset += LEN_DATA.LEN_MOT.value
    # rx_queue.put(f"M0 : {mot0_data}")
    # # Mot 1
    # mot1_data = struct.unpack('<h', bytes(packet_data[offset:offset+2]))[0]#(packet_data[offset + 1] << 8) | packet_data[offset]
    # offset += LEN_DATA.LEN_MOT.value
    # rx_queue.put(f"M1 : {mot1_data}")
    # # p et m, équation de la droite mx+p
    # p = float(packet_data[offset:offset+LEN_DATA.LEN_FLOAT.value-1].decode('utf-8', errors='ignore').strip())
    # offset += LEN_DATA.LEN_FLOAT.value
    # m = float(packet_data[offset:offset+LEN_DATA.LEN_FLOAT.value-1].decode('utf-8', errors='ignore').strip())
    # offset += LEN_DATA.LEN_FLOAT.value
    # # angle
    # angle = float(packet_data[offset:offset+LEN_DATA.LEN_FLOAT.value-1].decode('utf-8', errors='ignore').strip())
    # offset += LEN_DATA.LEN_FLOAT.value
    # # p et m aplatis, équation de la droite mx+p
    # p_aplati = float(packet_data[offset:offset+LEN_DATA.LEN_FLOAT.value-1].decode('utf-8', errors='ignore').strip())
    # offset += LEN_DATA.LEN_FLOAT.value
    # m_aplati = float(packet_data[offset:offset+LEN_DATA.LEN_FLOAT.value-1].decode('utf-8', errors='ignore').strip())
    # offset += LEN_DATA.LEN_FLOAT.value
    # angle_aplati = float(packet_data[offset:offset+LEN_DATA.LEN_FLOAT.value-1].decode('utf-8', errors='ignore').strip())
    # offset += LEN_DATA.LEN_FLOAT.value
    # # Image
    # data = bytearray()  # Réinitialiser les données pour une nouvelle image
    # # print([format(b, '08b') for b in packet_data[offset:]])
    # for index in range(offset, offset + LEN_DATA.LEN_IMG.value):
    #     huit_pixels = packet_data[index]
    #     for i in range(8):  # Traiter chaque bit
    #         pixel_value = 255 if (huit_pixels & (1 << (7 - i))) else 0
    #         data.append(pixel_value)
    # # color = 1  # Méthode alternative (RLE)
    # # for index in range(offset, len(packet_data), 2):
    # #     nb_pixels = (packet_data[index] << 8) | packet_data[index + 1]
    # #     new_data = bytearray([0 if color == 0 else 255] * nb_pixels)
    # #     data.extend(new_data)
    # #     color = 1 - color  # Alterner entre noir et blanc


def recv_exact(sock, n):
    """ Force la lecture de exactement n octets """
    buffer = bytearray()
    while len(buffer) < n:
        ready = select.select([sock], [], [], 2)
        if ready[0]:
            packet = sock.recv(n - len(buffer))
            if not packet:
                print("[TCP] Connexion fermée par le serveur pendant la réception.")
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
    
    if packet_size == 0:
        print(header, packet_type, packet_size)
        return bytes(data), packet_type

    # 2. Lire le contenu exact du paquet
    packet_data = recv_exact(sock, packet_size)
    # print(f"[TCP] Données reçues : {len(packet_data) if packet_data else 0} octets (attendu {packet_size} octets)")
    if packet_data is None:
        print(header, packet_type, packet_size)
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
    elif packet_type == DATA_TYPE.GENERAL.value:
        rx_queue.put(f"{packet_data.decode('utf-8', errors='ignore').strip()}")
    # print(f"[TCP] Paquet de type {packet_type} traité, données accumulées : {len(data)} octets.")
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
    if len(img.shape) == 2:  # Si l'image est en niveaux de gris
        img_bgr = cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)
    else:
        img_bgr = img.copy()
    # Coordonnées de l'image (origine en bas à gauche)
    height = img.shape[0]
    width = img.shape[1]
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




def tcp_thread(sock):
    global running, temps_debut, log, p, m, angle, p_aplati, m_aplati, angle_aplati
    print("[TCP] Thread démarré.")

    # Calculer la matrice de transformation
    # Points sources (exemple : coins de l'image distordue)
    source_points = [(0, 0), (80, 0), (0, 60), (80, 60)]

    # Points de destination (exemple : coins de l'image redressée)
    destination_points = [(0, 0), (140, 0), (30, 190), (110, 190)]

    M = cv2.getPerspectiveTransform(np.float32(source_points), np.float32(destination_points))

    while running:
        try:
            temps_debut = time.time()
            raw, packet_type = receive_data(sock)
            if packet_type == DATA_TYPE.END_IMG.value or packet_type == DATA_TYPE.ALL_IN_ONE.value:
                if len(raw) != IMAGE_SIZE:
                    print(f"[TCP] Attention : image reçue de taille {len(raw)} octets (attendu {IMAGE_SIZE} octets).")
                    # print(f"[TCP] Raw data (bin) : {[format(b, '08b') for b in raw]}")
                    raw = raw + bytes([128] * (IMAGE_SIZE - len(raw)))
                # print(f"[TCP] Image reçue ({len(raw)} octets) en {time.time() - temps_debut:.2f} secondes.")
                img = decode_bw_image(raw)
                img_reconstructed = correct_perspective(img, M)

                img = draw_line_on_image(img, m, p)
                img = draw_line_on_image(img, np.tan(angle), 0, color=(255, 0, 0), thickness=1)
                _, buffer = cv2.imencode('.png', img)
                img_base64 = base64.b64encode(buffer).decode('utf-8')
                rx_queue.put(f"IMG_DATA:{img_base64}")
                
                img_reconstructed = draw_line_on_image(img_reconstructed, m_aplati, p_aplati)
                img_reconstructed = draw_line_on_image(img_reconstructed, np.tan(angle_aplati), 0, color=(255, 0, 0), thickness=1)
                # img_reconstructed = draw_line_on_image(img_reconstructed, m, p)
                # img_reconstructed = draw_line_on_image(img_reconstructed, np.tan(angle), 0, color=(255, 0, 0), thickness=1)
                _, buffer = cv2.imencode('.png', img_reconstructed)
                img_base64 = base64.b64encode(buffer).decode('utf-8')
                rx_queue.put(f"IMG_DATA_R:{img_base64}")
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