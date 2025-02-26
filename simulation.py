import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.animation import FuncAnimation
import random

def normal1ize(v):
    """ normal1isation d'un vecteur """
    return v / np.linalg.norm(v)

def reflect(ray_dir, normal1):
    """ Calcule la réflexion d'un rayon sur une surface """
    ray_dir = normal1ize(ray_dir)
    normal1 = normal1ize(normal1)
    return normal1ize(ray_dir - 2 * np.dot(ray_dir, normal1) * normal1)

# Paramètres de la scène
plane_size = 20  # Taille plus grande du plan pour s'assurer que l'impact est visible
normal1 = np.array([0, 1, 0])  # normal1e du plan (y = 0 => plan horizontal)

# Création de la figure
fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')

xx, zz = np.meshgrid(np.linspace(-plane_size, plane_size, 20), np.linspace(-plane_size, plane_size, 20))
yy = np.zeros_like(xx)
ax.plot_surface(xx, yy, zz, alpha=0.5, color='gray')



# Initialisation des éléments graphiques
light_pos = np.array([0, 5, 3])  # Position initiale
light_scatter = ax.scatter(*light_pos, color="yellow", s=100, label="Source lumineuse")
impact_scatter = ax.scatter(0, 0, 0, color="black", s=100, label="Point d'impact")
ray_line, = ax.plot([], [], [], 'r-', label="Rayon incident")
reflected_line, = ax.plot([], [], [], 'b-', label="Rayon réfléchi")

# Fonction de mise à jour pour l'animation
def update(frame):
    global light_pos

    # Déplacement aléatoire de la lumière
    light_pos[0] = random.uniform(-5, 5)
    light_pos[1] = 5 + 2 * np.sin(frame / 30)
    light_pos[2] = random.uniform(-5, 5)

    # Direction du rayon (vers le bas)
    ray_dir = np.array([light_pos[0], -1, light_pos[2]])

    # Calcul de l'intersection avec le plan (y=0)
    t = -light_pos[1] / ray_dir[1]
    impact_point = light_pos + t * ray_dir

    # Vérification si le point d'impact est bien sur la zone visible
    if np.abs(impact_point[0]) > plane_size or np.abs(impact_point[2]) > plane_size:
        impact_point = np.array([0, 0, 0])  # Déplacer le point à l'origine si hors zone

    # Calcul de la réflexion correcte
    reflected_dir = reflect(ray_dir, normal1)
    reflected_point = impact_point + reflected_dir * 5  # Augmenter la portée du rayon réfléchi

    # Mise à jour des graphiques
    light_scatter._offsets3d = ([light_pos[0]], [light_pos[1]], [light_pos[2]])
    impact_scatter._offsets3d = ([impact_point[0]], [impact_point[1]], [impact_point[2]])

    ray_line.set_data([light_pos[0], impact_point[0]], [light_pos[1], impact_point[1]])
    ray_line.set_3d_properties([light_pos[2], impact_point[2]])

    reflected_line.set_data([impact_point[0], reflected_point[0]], [impact_point[1], reflected_point[1]])
    reflected_line.set_3d_properties([impact_point[2], reflected_point[2]])

    return light_scatter, impact_scatter, ray_line, reflected_line

# Lancement de l'animation
ani = FuncAnimation(fig, update, frames=200, interval=1, blit=False)

# Affichage
ax.set_xlabel("X")
ax.set_ylabel("Y")
ax.set_zlabel("Z")
ax.set_title("Réflexion dynamique de la lumière")
ax.legend()
ax.view_init(elev=20, azim=30)  # Angle de vue

plt.show()
