1. **Etapa 1. Preparación de la ejecución CGI**
    **Objetivo:** Construir los datos que necesitará el proceso hijo.
    - Selección del intérprete y script.
    - Construcción de `argv`.
    - Construcción de `envp`.
    - Variables CGI: `REQUEST_METHOD`, `SCRIPT_NAME`, `QUERY_STRING`, `CONTENT_LENGTH`, etc.
2. **Etapa 2. Comunicación entre procesos**
    **Objetivo:** Implementar la creación del proceso CGI y sus canales de comunicación.
    - `fork()` y `execve()`.
    - `pipe()`, `dup2()` y `close()`.
    - Entrada del script mediante `stdin`.
    - Salida del script mediante `stdout`.
    - Directorio de trabajo y cierre correcto de descriptores.
3. **Etapa 3. Comunicación no bloqueante con el Event Loop**
    **Objetivo:** Integrar los pipes CGI con el mecanismo de multiplexación que ya utiliza Webserv.
    - Escritura progresiva del body HTTP hacia CGI.
    - Lectura progresiva de la salida CGI.
    - Gestión simultánea de ambos canales.
    - EOF, desconexiones y ciclo de vida del proceso.
4. **Etapa 4. Parser de respuestas CGI**
    **Objetivo:** Interpretar correctamente lo que devuelve el script.
    - Separación de cabeceras y cuerpo.
    - `Content-Type`, `Status` y `Location`.
    - Respuestas normales y redirecciones.
    - Cabeceras incorrectas, duplicadas o incompletas.
    - Cuerpos binarios y lecturas parciales.
5. **Etapa 5. Integración con HttpResponse**
    **Objetivo:** Transformar el resultado CGI en una respuesta HTTP válida.
    - Entrega del resultado al Response Builder.
    - Tratamiento de códigos HTTP.
    - Gestión de cabeceras y cuerpo.
    - Respuestas de error.
6. **Etapa 6. Robustez y gestión de errores**
    **Objetivo:** Completar los casos que pueden comprometer la estabilidad del servidor.
    - Timeouts.
    - Fallos de `execve()`.
    - Procesos que terminan incorrectamente.
    - Scripts que no producen una respuesta válida.
    - Limpieza de procesos y descriptores.
    - Clientes que se desconectan.
7. **Etapa 7. Validación completa**
    **Objetivo:** Comprobar el CGI dentro del Webserv terminado.
    - GET y QUERY_STRING.
    - POST y transferencia del body.
    - Más de un intérprete o programa CGI.
    - Respuestas grandes y binarias.
    - Varias peticiones CGI simultáneas.