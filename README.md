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

### CMake (Linux) :
Dans un répertoire au niveau de CMakeLists.txt :  
```bash
mkdir build
cd build
cmake .. -DPICO_BOARD=pico2_w
make -j
./DEMO
```


## Structure du projet :  
.  
├── **Divers** : contient les flashs Micropython et quelques documents  
├── **IHM** : développement de l'IHM  
├── **Integration** : Dossier principal contenant le code complet  
├── **Moteurs** : développement du pilotage par PWM  
├── **Support** : développement du support caméra (impression 3D)  
└── **TraitementImage** : développement de la capture et du traitement d'image  




