# -*- coding: utf-8 -*-
"""
Created on Thu Oct  9 15:02:35 2025

@author: Alexis
"""

import socket
import time

# Adresse IP du Pico (remplace par l'IP réelle)
PICO_IP = "172.20.10.2"
# "192.168.31.242"
# "192.168.1.43"
# Port du serveur TCP sur le Pico
PICO_PORT = 4242


def main():
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
    
    while True:
        cpt += 1
        try:
            data = sock.recv(512)
            print(f"Reçu: {data}")
            message = f'Hello {cpt} !'.encode('utf-8')
            time.sleep(1)
            print(cpt)
            sock.send(message)
    
        except:
            # closure_message = b'0000'
            # sock.send(closure_message)
            sock.close()
            break
        


if __name__ == "__main__":
    main()
    
