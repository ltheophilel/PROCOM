from flask import Flask, render_template, request, jsonify
import serial
import serial.tools.list_ports
import atexit
import random

# Recherche automatique du port du Pico
def find_pico_port():
    ports = serial.tools.list_ports.comports()
    print(ports)
    for port in ports:
        print(port.device)
        print(port.description)
        if "Pico" in port.description or "USB" in port.description:
            return port.device
    return None


pico_port = find_pico_port()
if not pico_port:
    print("⚠️ Pico non détecté. Branchez-le en USB puis relancez.")
    # quit()
else:
    print("Pico détecté")

# Connexion série
ser = serial.Serial(pico_port, 115200, timeout=1)
def close_serial():
    if ser.is_open:
        print("🔌 Fermeture du port série...")
        ser.close()

atexit.register(close_serial)

app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/send', methods=['POST'])
def send():
    data = request.json.get('command')
    ser.write((data + '\n').encode())
    print(data)
    print("hello")
    return jsonify({"status": "sent", "command": data})

@app.route('/vitesse_mot', methods=['POST'])
def vitesse_mot():
    v_mot = random.uniform(0, 100)  # Simule une vitesse aléatoire entre 0 et 100
    return jsonify(v_mot)

@app.route('/read', methods=['GET'])
def read():
    line = ser.readline().decode(errors='ignore').strip()
    return jsonify({"data": line})

if __name__ == '__main__':
    print(f"✅ Serveur lancé sur http://localhost:5000 (Pico sur {pico_port})")
    app.run(host='0.0.0.0', port=5000, debug=False)
