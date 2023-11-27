import numpy as np
import sys
import folium

# fitxer d'entrada
in_file = sys.argv[1]

# nom del fitxer de sortida: el mateix que l'entrada, canviant extensio per html
mapfile = in_file.split('.')[-2] + ".html"

# convertim les dades del fitxer en un array de numpy
xy_array = np.genfromtxt(in_file, delimiter="|", skip_header=3)
xy = xy_array.tolist()

# Obtenim les coordenades de les llistes
coordinates_list = [(point[1], point[2]) for point in xy]

# Crea un mapa centrado en la primera coordenada
m = folium.Map(location=[xy[0][1], xy[0][2]], zoom_start=15)

# Afegeix una línia al mapa amb les coordenades
folium.PolyLine(locations=coordinates_list, color='blue').add_to(m)

# Guarda el mapa com a fitxer HTML
m.save(mapfile)
