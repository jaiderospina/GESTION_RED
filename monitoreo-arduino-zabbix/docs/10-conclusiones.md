# 10. Conclusiones

1. **Se integró con éxito una estación IoT de bajo costo a una plataforma de monitoreo empresarial.** Zabbix 7.0 trata al Arduino como un equipo más de la red: recoge seis métricas cada minuto, guarda su historial y evalúa siete iniciadores. Así se confirma que las herramientas de gestión de red sirven también para dispositivos de campo.

2. **La alternativa USB aprobada fue más simple y robusta que el Ethernet Shield** para un laboratorio con un único servidor. El firmware conserva el modo Ethernet (agente pasivo con protocolo JSON de Zabbix 7.0), así que el proyecto puede pasar a red sin cambiar la plantilla: solo cambian la IP de la interfaz y una constante.

3. **El ajuste de umbrales con datos reales es indispensable.** Los valores teóricos iniciales (50 lx para "baja luz", `min(#3)` para ruido) generaban una alarma permanente o ninguna. Al calibrar con los gráficos (fondo de ruido de 8 a 40, picos de 380 a 607; luz ambiente de 5 a 20 lx) las alertas pasaron a reflejar eventos reales.

4. **Las dependencias entre iniciadores y la histéresis reducen el ruido de alertas.** Cuando la estación se desconectó (16-nov), Zabbix informó un solo problema ("sin datos") en lugar de una cascada. Las expresiones de recuperación con margen evitan el "aleteo" de los problemas.

5. **Telegram resultó un canal de notificación eficaz:** el aviso llega en segundos al celular, con el valor medido (datos operativos) y con el mensaje de resolución, sin depender de servidores de correo.

6. **La seguridad hay que manejarla en todo el ciclo.** SELinux se mantuvo activo y solo se relajó el dominio del agente. Las contraseñas por defecto se cambiaron. Un token de bot expuesto se revocó, y el repositorio excluye cualquier secreto.

7. **La bitácora de incidentes mostró problemas típicos de operación:** puertos serie que cambian de nombre, políticas de SELinux, rutas de repositorio y configuraciones que no se guardaron. Cada uno dejó una mejora permanente, como la detección automática del puerto.

## Trabajo futuro

- Revisar el ajuste del PIR (problema de movimiento permanente) y agregar el sensor DHT11 de temperatura y humedad.
- Crear una regla `udev` (`/dev/arduino`) para un nombre de puerto estable.
- Traducir las plantillas de mensaje de Telegram al español.
- Recolectar al menos 72 h de datos para analizar los patrones día/noche y de ocupación del salón.
- Probar el modo Ethernet con el Shield W5100 y comparar la latencia y la confiabilidad.
