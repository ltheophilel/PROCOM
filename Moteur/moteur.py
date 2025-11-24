from machine import Pin, PWM
import time

# Pinos conectados à PmodHB5
DIR = Pin(14, Pin.OUT)      # Define o sentido de rotação
EN = PWM(Pin(15))           # PWM para velocidade

# Configuração da frequência do PWM
EN.freq(20000)              # 20 kHz (motor fica silencioso)

print("Iniciando teste do motor...")

# 1) Define direção (HORÁRIO)
DIR.low()
print("Direção: HORÁRIO")

# 2) Liga o motor a 50% da velocidade
EN.duty_u16(32768)          # valor entre 0 e 65535
print("Motor LIGADO a 50%...")

# Mantém ligado por 3 segundos
time.sleep(3)

# 3) Para o motor
EN.duty_u16(0)
print("Motor DESLIGADO.")
