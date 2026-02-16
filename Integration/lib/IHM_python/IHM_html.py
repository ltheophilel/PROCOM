# ====================== routes Flask ======================
import threading
from flask import Flask, request, jsonify, render_template

app = Flask(__name__)

sock = None
sock_lock = threading.Lock()

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
    global rx_queue
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