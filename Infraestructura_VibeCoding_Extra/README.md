# Infraestructura local (broker + Node-RED + base de datos)

Todo el "lado servidor" del proyecto corre en tu propia PC con Docker. No se
usa ninguna nube ni ngrok: el ESP32 (Wokwi o físico) y el celular Android se
conectan al broker MQTT que vive en esta misma máquina.

## Componentes

| Servicio    | Imagen                | Puerto | Para qué sirve                                              |
|-------------|-----------------------|--------|-------------------------------------------------------------|
| `mosquitto` | eclipse-mosquitto:2   | 1883   | Broker MQTT. Por él pasan todos los mensajes.               |
| `nodered`   | nodered/node-red:4.0  | 1880   | Lógica + **historial de activaciones en SQLite** + panel web. |
| `influxdb`  | influxdb:2.7          | 8086   | (Opcional) Base de series de tiempo para la aceleración.    |
| `telegraf`  | telegraf:1.31         | —      | (Opcional) Copia los mensajes MQTT hacia InfluxDB.          |

InfluxDB y Telegraf están bajo el perfil `telemetria` y son **opcionales**: el
requisito de "historial de activaciones" lo cumple Node-RED con SQLite, que es
mucho más simple y robusto de demostrar.

## Cómo levantarlo

Requisito: tener **Docker Desktop** instalado y abierto.

```bash
cd Infraestructura

# Esencial (broker + Node-RED + historial SQLite):
docker compose up -d --build

# (Opcional) agregar series de tiempo InfluxDB + Telegraf:
docker compose --profile telemetria up -d --build
```

Para detener todo: `docker compose down` (agregar `-v` para borrar también los
volúmenes/datos).

## Qué queda disponible

- **Panel web del proyecto:** http://localhost:1880/alarma
  Muestra el estado actual de la alarma, la magnitud de aceleración, botones
  ARMAR / DESARMAR / PÁNICO y la tabla del historial de activaciones (se refresca
  solo cada 2 s).
- **Editor de Node-RED:** http://localhost:1880
- **Historial en JSON:** http://localhost:1880/alarma/historial
- **Enviar comando por URL:** http://localhost:1880/alarma/comando?cmd=ARM
- **InfluxDB (si está activo):** http://localhost:8086
  Usuario `admin` / clave `adminadmin`, organización `default`, bucket `sensores_db`.

## Topicos MQTT

| Tópico            | Dirección        | Contenido (JSON)                                         |
|-------------------|------------------|----------------------------------------------------------|
| `alarma/estado`   | ESP32 → mundo    | `{"estado":"ACTIVO","modo":"ARMADO","codigo":1}`         |
| `alarma/acel`     | ESP32 → mundo    | `{"magnitud":9.8,"x":..,"y":..,"z":..,"referencia":..}`  |
| `alarma/evento`   | ESP32 → mundo    | `{"evento":"MOVIMIENTO","detalle":"ADVERTENCIA"}`        |
| `alarma/comando`  | mundo → ESP32    | `ARM` / `DISARM` / `PANIC` (texto) o `{"cmd":"ARM"}`     |

## Credenciales del broker

Usuario `claromio`, clave `RiverBest` (definidas en `mosquitto/config/passwd`).
Las usan el ESP32 (`config.h`), la app Android y Node-RED (precargadas en
`nodered/data/flows_cred.json`).

> Para cambiar la clave: `docker exec -it mosquitto_broker mosquitto_passwd -b /mosquitto/config/passwd claromio NUEVACLAVE` y reiniciar el contenedor.

## La base de datos del historial

Node-RED crea automáticamente, al desplegar el flujo, la tabla `activaciones`
en `nodered/data/historial.db` (SQLite):

```sql
CREATE TABLE IF NOT EXISTS activaciones (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  ts TEXT DEFAULT (datetime('now','localtime')),
  tipo TEXT,      -- 'estado' o 'evento'
  estado TEXT,    -- APAGADO / ACTIVO / ADVERTENCIA / ALERTADO
  modo TEXT,      -- DESARMADO / ARMADO / VIGILANDO / ALARMA
  detalle TEXT,
  origen TEXT
);
```

Cada cambio de estado y cada evento que publica el ESP32 inserta una fila. Podés
inspeccionarla con cualquier visor de SQLite o desde el contenedor:

```bash
docker exec -it node_red node -e "const db=require('sqlite3');" # (o usar el panel /alarma)
```
