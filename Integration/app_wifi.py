from flask import Flask, request, jsonify, render_template
import socket
import threading
import queue
import time

PICO_IP = "172.20.10.2"
PICO_PORT = 4242

sock = None
sock_lock = threading.Lock()
running = True

# file de réception non bloquante
rx_queue = queue.Queue()

app = Flask(__name__)


def tcp_thread():
    global sock, running

    print("[TCP] Thread démarré.")

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(5)

    print(f"[TCP] Connexion à {PICO_IP}:{PICO_PORT}…")
    sock.connect((PICO_IP, PICO_PORT))
    sock.settimeout(None)
    print("[TCP] Connecté au Pico.")

    while running:
        try:
            data = sock.recv(512)
            text = data.decode("utf-8", errors="ignore").strip()
            rx_queue.put(text)

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
    if not rx_queue.empty():
        msgs = []
        all_msgs = {}
        while not rx_queue.empty():
            msg = rx_queue.get()
            print("[WEB] Message reçu :", msg)
            if msg.startswith("M0 :"): 
                v_mot = msg.split("M0 :")[1].strip()
                all_msgs["v_mot0"] = v_mot
            elif msg.startswith("M1 :"):
                v_mot = msg.split("M1 :")[1].strip()
                all_msgs["v_mot1"] = v_mot
            else:
                msgs.append(msg)
                all_msgs["data"] = msgs
        return jsonify(all_msgs)

    return jsonify({"data": None})


if __name__ == "__main__":
    # thread TCP
    t = threading.Thread(target=tcp_thread)
    t.start()

    # démarrage Flask
    print("[WEB] Démarrage serveur…")
    app.run(host="0.0.0.0", port=5000, debug=False)

    running = False
    t.join()
