import matplotlib.pyplot as plt
import numpy as np

alpha_degres = np.linspace(-90, 90, 180).astype(int)
vmax = 100;
vitesse_droite = vmax/2 * (1 + alpha_degres/90) 
vitesse_gauche = vmax/2 * (1 - alpha_degres/90)

plt.plot(alpha_degres, alpha_degres, alpha_degres/90, np.sin(alpha_degres*np.pi/180))
plt.legend(["Moteur Droit", "Moteur Gauche"])
plt.show()

