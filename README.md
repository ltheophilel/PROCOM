# PROCOM : Pilotage plateforme mobile 
![Interface](/Divers/robot_cote.jpg)

## Citation
Camera OV7670 code inspiré de :  
https://github.com/usedbytes/camera-pico-ov7670 (BSD-3 Clause)

## Matériel
+ Caméra OV7670
+ Microcontrôleur Raspberry Pi Pico 2W
+ Ecran LCD
+ Moteurs à courant continu (https://digilent.com/reference/motor_gearbox/motor_gearbox)
+ Drivers (Digilent PmodHB5TM 2A H-Bridge Module)  


## Compilation

### VS Code Raspberry Pi Pico Extension :
- Uploader le code sur la carte : la brancher sur l’ordinateur en mode BOOTSEL (laisser le
bouton RESET de la carte appuyée en la branchant). La carte devrait être détectée par
l’ordinateur, comme une clé USB. Ouvrez le dossier du projet sur VSCode, sélectionnez
l’extension dans la barre latérale et sélectionnez “Run Project (USB)”. La compilation va
s’effectuer, puis le téléversement sur la carte.
- Configurer le wifi : allez dans lib ► wifi ► include ► wifi_login.h, et ajoutez un élément à
known_networks, sous la forme {“Nom du réseau”, “mot de passe”}. Si le réseau ne
demande pas de mot de passe, mettre NULL à la place du mot de passe. Les réseaux avec
certificats ne sont pas supportés.
- Uploader sur une Raspberry Pi Pico W : bien que n’ayant testé que sur une Raspberry Pi
Pico 2 W, nous pensons que notre code peut fonctionner sur la version antérieure. Pour
permettre la compilation, changer la ligne 26 de CMakeLists.txt :
set(PICO_BOARD pico2_w CACHE STRING "Board type")
en :
set(PICO_BOARD pico_w CACHE STRING "Board type")
Puis, dans le panneau de l’extension, sélectionnez “Clean CMake”. Attendre la fin de
l’opération, puis uploadez normalement.
- Recevoir les messages de debug de la carte : avec la carte branchée à un ordinateur
(pas en BOOTSEL), et VSCode ouvert, aller dans Serial Monitor (barre inférieure),
sélectionner le port COM correspondant à la carte et sélectionner Start Monitoring.

### CMake (Linux) :
Dans un répertoire au niveau de CMakeLists.txt :  
```bash
mkdir build
cd build
cmake .. -DPICO_BOARD=pico2_w
make -j
```
Puis brancher le Raspberry Pico en BOOTSEL et y copier le fichier .uf2.   

## Structure du projet :  
.  
├── **Divers** : contient les flashs Micropython et quelques documents  
├── **IHM** : développement de l'IHM  
├── **Integration** : Dossier principal contenant le code complet  
├── **Moteurs** : développement du pilotage par PWM  
├── **Modeles** : développement du support caméra (impression 3D)  
└── **TraitementImage** : développement de la capture et du traitement d'image  




