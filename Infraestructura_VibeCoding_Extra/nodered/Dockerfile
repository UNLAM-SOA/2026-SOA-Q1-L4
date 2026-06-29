# Imagen de Node-RED con los nodos extra que usa el flujo:
#  - node-red-node-sqlite : base de datos del historial de activaciones
# (El nodo http/template/mqtt/function ya viene de fabrica en Node-RED.)
FROM nodered/node-red:4.0

# Se instalan los nodos adicionales dentro de la imagen.
RUN npm install --unsafe-perm --no-update-notifier --no-fund node-red-node-sqlite
