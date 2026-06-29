/**
 * settings.js - Configuracion de Node-RED para el proyecto.
 *
 * credentialSecret:false  -> guarda las credenciales (usuario/clave del broker)
 *   en texto plano en flows_cred.json. Esto permite precargar la conexion al
 *   broker Mosquitto sin tener que escribir la clave a mano en el editor.
 *   Es aceptable en este entorno LOCAL de desarrollo/laboratorio.
 */
module.exports = {
    flowFile: 'flows.json',
    credentialSecret: false,
    uiPort: process.env.PORT || 1880,
    logging: {
        console: { level: "info", metrics: false, audit: false }
    },
    exportGlobalContextKeys: false,
    functionGlobalContext: {},
    // El panel web del proyecto queda accesible en  http://localhost:1880/alarma
    httpNodeRoot: '/',
    editorTheme: {
        projects: { enabled: false }
    }
};
