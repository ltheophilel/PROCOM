
# import pyautogui
import socket
import threading
import webbrowser

all_ip = ["192.168.31.233", "172.20.10.2", "192.168.1.114"]

PICO_IP = all_ip[1]
PICO_PORT = 4242

# ====================== routes Flask ======================
from lib.IHM_python.IHM_html import app, sock

# ====================== TCP thread ======================
from lib.IHM_python.IHM_communication import tcp_thread, log
    
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
