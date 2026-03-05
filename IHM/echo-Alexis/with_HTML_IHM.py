from flask import Flask, render_template, request, jsonify
import serial
import serial.tools.list_ports
import atexit
import socket
import time

USB = 0
TCP = 1

ser = 0
sock = 0

MODE = TCP  # Choisir entre USB ou TCP

# Recherche automatique du port du Pico EN USB
def find_pico_port():
    ports = serial.tools.list_ports.comports()
    print(ports)
    for port in ports:
        print(port.device)
        print(port.description)
        if "Pico" in port.description or "USB" in port.description:
            return port.device
    return None

## Connexion au Pico en USB
def main_USB():
    pico_port = find_pico_port()
    if not pico_port:
        print("⚠️ Pico non détecté. Branche-le en USB puis relance.")
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
    return pico_port


def main_TCP():
    # Adresse IP du Pico (remplace par l'IP réelle)
    PICO_IP = "192.168.1.43"
    # Port du serveur TCP sur le Pico
    PICO_PORT = 4242

    # Crée un socket TCP
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    cpt = 0
    try:
        # Connexion au serveur TCP du Pico
        sock.connect((PICO_IP, PICO_PORT))
        print(f"Connecté à {PICO_IP}:{PICO_PORT}")
    except Exception as e:
        print(f"Erreur: {e}")
        # Ferme la connexion
        sock.close()
        print("Connexion fermée.")
    
    # while True:
    #     cpt += 1
    #     try:
    #         data = sock.recv(512)
    #         print(f"Reçu: {data}")
    #         message = f'Hello {cpt} !'.encode('utf-8')
    #         time.sleep(1)
    #         print(cpt)
    #         sock.send(message)
    
    #     except:
    #         # closure_message = b'0000'
    #         # sock.send(closure_message)
    #         sock.close()
    #         break



## Lie à l'HTML IHM

app = Flask(__name__)

@app.route('/')
def index():
    print("index")
    return render_template('index.html')

@app.route('/send', methods=['POST'])
def send():
    print("send")
    data = request.json.get('command')
    if MODE == TCP:
        print("tcp send")
        sock.send(data.encode('utf-8'))
    elif MODE == USB:
        ser.write((data + '\n').encode())
    print(data)
    print("hello")
    return jsonify({"status": "sent", "command": data})

@app.route('/read', methods=['GET'])
def read():
    print("read")
    if MODE == TCP:
        line = sock.recv(512)
    elif MODE == USB:
        line = ser.readline().decode(errors='ignore').strip()
    return jsonify({"data": line})

if __name__ == '__main__':
    pico_port = "TCP"
    if MODE == USB:
        pico_port = main_USB()
    if MODE == TCP:
        main_TCP()
    print(f"✅ Serveur lancé sur http://localhost:5000 (Pico sur {pico_port})")
    app.run(host='0.0.0.0', port=5000, debug=True)
    print("Serveur arrêté.")
