#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>

#define PORT 8080
#define MAX_EVENTS 10

int main()
{
	int					server_fd;
	int					new_socket;
	long				valread;
	struct sockaddr_in	address;
	socklen_t			addrlen;
	char				*hello;
	char				buffer[30000];
	int					epoll_fd;
	int					event_count;
	int					i;
	int					current_fd;
	int					timeout_ms;
	struct epoll_event	event;
	struct epoll_event	events[MAX_EVENTS];

	server_fd = -1;
	new_socket = -1;
	valread = 0;
	addrlen = sizeof(address);
	hello = "hola desde el servidor";
	timeout_ms = -1;

	/*
	 * =========================
	 * 1) CREACIÓN DEL SOCKET
	 * =========================
	 *
	 * Creamos el socket TCP del servidor.
	 *
	 * Este FD únicamente se utilizará para:
	 *
	 *     recibir conexiones
	 *     accept()
	 *
	 * No representa todavía ningún cliente.
	 */
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	/*
	 * =========================
	 * 2) CONFIGURACIÓN DIRECCIÓN
	 * =========================
	 */
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	/*
	 * =========================
	 * 3) BIND
	 * =========================
	 *
	 * Asociamos server_fd con:
	 *
	 *     0.0.0.0:8080
	 */
	if (bind(server_fd, (struct sockaddr *)&address,
			sizeof(address)) < 0)
	{
		perror("bind");
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	/*
	 * =========================
	 * 4) LISTEN
	 * =========================
	 *
	 * server_fd pasa a ser un socket de escucha.
	 *
	 * El backlog 10 indica cuántas conexiones
	 * pueden permanecer pendientes esperando
	 * un accept().
	 */
	if (listen(server_fd, 10) < 0)
	{
		perror("listen");
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	/*
	 * =========================================================
	 * 5) CREAR LA INSTANCIA EPOLL
	 * =========================================================
	 *
	 * epoll_create1() crea una instancia de epoll dentro
	 * del kernel.
	 *
	 * Esta instancia mantendrá la lista de FDs que queremos
	 * vigilar.
	 *
	 * Podemos imaginarlo así:
	 *
	 *     epoll_fd
	 *         |
	 *         +--- server_fd
	 *         +--- cliente 1
	 *         +--- cliente 2
	 *         ...
	 *
	 * El argumento 0 significa que no usamos flags especiales.
	 *
	 * RETORNO:
	 *
	 *     >= 0  FD que representa la instancia epoll
	 *     -1    error
	 */
	epoll_fd = epoll_create1(0);
	if (epoll_fd < 0)
	{
		perror("epoll_create1");
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	/*
	 * =========================================================
	 * 6) PREPARAR EL EVENTO DEL SERVER_FD
	 * =========================================================
	 *
	 * struct epoll_event tiene principalmente:
	 *
	 *     events
	 *         qué queremos vigilar.
	 *
	 *     data
	 *         información que queremos asociar al FD.
	 *
	 * Aquí decimos:
	 *
	 *     quiero vigilar EPOLLIN
	 *
	 * sobre:
	 *
	 *     server_fd
	 *
	 * En un socket de escucha:
	 *
	 *     EPOLLIN
	 *
	 * significa:
	 *
	 *     hay al menos una conexión pendiente
	 *     que podemos aceptar con accept().
	 */
	memset(&event, 0, sizeof(event));
	event.events = EPOLLIN;
	event.data.fd = server_fd;

	/*
	 * =========================================================
	 * 7) REGISTRAR SERVER_FD EN EPOLL
	 * =========================================================
	 *
	 * epoll_ctl() modifica la lista de interés de epoll.
	 *
	 * EPOLL_CTL_ADD:
	 *
	 *     añade un nuevo FD a la instancia.
	 *
	 * Por tanto estamos diciendo:
	 *
	 *     "epoll_fd, empieza a vigilar server_fd
	 *      y avísame cuando tenga EPOLLIN".
	 *
	 * RETORNO:
	 *
	 *     0   éxito
	 *    -1   error
	 */
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD,
			server_fd, &event) < 0)
	{
		perror("epoll_ctl server_fd");
		close(epoll_fd);
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	printf("Servidor escuchando en el puerto %d\n", PORT);

	while (1)
	{
		printf("\n+++++++ Waiting for activity (epoll) ++++++++\n\n");

		/*
		 * =====================================================
		 * 8) ESPERAR EVENTOS CON EPOLL_WAIT
		 * =====================================================
		 *
		 * epoll_wait() espera hasta que alguno de los FDs
		 * registrados tenga actividad.
		 *
		 * Argumentos:
		 *
		 * epoll_fd
		 *     instancia de epoll.
		 *
		 * events
		 *     array donde epoll guardará los eventos
		 *     que realmente ocurrieron.
		 *
		 * MAX_EVENTS
		 *     máximo de eventos que puede devolver
		 *     en esta llamada.
		 *
		 * timeout_ms
		 *     -1 = esperar indefinidamente.
		 *
		 *
		 * IMPORTANTE:
		 *
		 * Con poll teníamos:
		 *
		 *     events
		 *     revents
		 *
		 * En epoll la idea se separa:
		 *
		 *     event.events
		 *
		 * se usó al registrar el FD para decir qué queremos.
		 *
		 * Después:
		 *
		 *     events[i].events
		 *
		 * contiene lo que realmente ocurrió.
		 *
		 *
		 * RETORNO:
		 *
		 *     > 0 número de eventos preparados
		 *       0 timeout
		 *      -1 error
		 */
		event_count = epoll_wait(epoll_fd, events,
				MAX_EVENTS, timeout_ms);
		if (event_count < 0)
		{
			perror("epoll_wait");
			close(epoll_fd);
			close(server_fd);
			exit(EXIT_FAILURE);
		}

		/*
		 * =====================================================
		 * 9) RECORRER SOLO LOS EVENTOS QUE OCURRIERON
		 * =====================================================
		 *
		 * Esta es una diferencia importante respecto a poll.
		 *
		 * Si epoll_wait devuelve:
		 *
		 *     event_count = 2
		 *
		 * significa que:
		 *
		 *     events[0]
		 *     events[1]
		 *
		 * contienen eventos válidos.
		 *
		 * No necesitamos recorrer todos los sockets
		 * registrados en epoll.
		 */
		i = 0;
		while (i < event_count)
		{
			/*
			 * Obtenemos el FD asociado al evento.
			 *
			 * Puede ser:
			 *
			 *     server_fd
			 *
			 * o:
			 *
			 *     un FD de cliente.
			 */
			current_fd = events[i].data.fd;

			/*
			 * =================================================
			 * 10) ¿EL EVENTO ES DEL SOCKET DEL SERVIDOR
			 * =================================================
			 *
			 * Si current_fd == server_fd significa que
			 * el evento pertenece al socket de escucha.
			 *
			 * Y si contiene EPOLLIN:
			 *
			 *     existe una conexión pendiente.
			 */
			if (current_fd == server_fd
				&& (events[i].events & EPOLLIN))
			{
				/*
				 * accept() saca una conexión de la cola
				 * de listen().
				 *
				 * El nuevo descriptor representa
				 * exclusivamente a ese cliente.
				 */
				new_socket = accept(server_fd,
						(struct sockaddr *)&address,
						&addrlen);
				if (new_socket < 0)
					perror("accept");
				else
				{
					printf("Nueva conexión aceptada (fd=%d)\n",
						new_socket);

					/*
					 * =============================================
					 * 11) REGISTRAR EL CLIENTE EN EPOLL
					 * =============================================
					 *
					 * Ya hemos aceptado el cliente.
					 *
					 * Ahora tenemos que decirle a epoll:
					 *
					 *     "vigila también este nuevo FD".
					 *
					 * Queremos EPOLLIN porque queremos saber
					 * cuándo el cliente tenga datos disponibles.
					 */
					memset(&event, 0, sizeof(event));
					event.events = EPOLLIN;
					event.data.fd = new_socket;

					/*
					 * Añadimos new_socket a la lista
					 * de interés de epoll.
					 *
					 * Antes:
					 *
					 *     epoll
					 *       |
					 *       +-- server_fd
					 *
					 * Después:
					 *
					 *     epoll
					 *       |
					 *       +-- server_fd
					 *       |
					 *       +-- new_socket
					 */
					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD,
							new_socket, &event) < 0)
					{
						perror("epoll_ctl client");
						close(new_socket);
					}
				}
			}

			/*
			 * =================================================
			 * 12) EL EVENTO PERTENECE A UN CLIENTE
			 * =================================================
			 *
			 * Si current_fd NO es server_fd:
			 *
			 *     significa que es un socket aceptado
			 *     anteriormente mediante accept().
			 */
			else
			{
				/*
				 * =============================================
				 * ERROR O CIERRE
				 * =============================================
				 *
				 * EPOLLERR:
				 *     ocurrió un error.
				 *
				 * EPOLLHUP:
				 *     ocurrió una condición de cierre.
				 *
				 * Usamos & porque events puede contener
				 * varias banderas simultáneamente.
				 */
				if (events[i].events & (EPOLLERR | EPOLLHUP))
				{
					printf("Cliente desconectado o error fd=%d\n",
						current_fd);

					/*
					 * =========================================
					 * ELIMINAR EL CLIENTE DE EPOLL
					 * =========================================
					 *
					 * EPOLL_CTL_DEL elimina el FD de la
					 * lista de interés.
					 *
					 * Después epoll dejará de vigilarlo.
					 */
					if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL,
							current_fd, NULL) < 0)
						perror("epoll_ctl del");

					close(current_fd);
				}

				/*
				 * =============================================
				 * DATOS DISPONIBLES DEL CLIENTE
				 * =============================================
				 *
				 * EPOLLIN en un socket de cliente significa:
				 *
				 *     hay datos disponibles para read()
				 *
				 * o el cliente cerró limpiamente y read()
				 * devolverá 0.
				 */
				else if (events[i].events & EPOLLIN)
				{
					memset(buffer, 0, sizeof(buffer));

					valread = read(current_fd, buffer,
							sizeof(buffer) - 1);

					if (valread < 0)
					{
						perror("read");

						/*
						 * Dejamos de vigilar este cliente.
						 */
						if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL,
								current_fd, NULL) < 0)
							perror("epoll_ctl del");

						close(current_fd);
					}
					else if (valread == 0)
					{
						printf("El cliente cerró la conexión "
							"(fd=%d)\n", current_fd);

						/*
						 * read() == 0 significa EOF:
						 * el otro extremo cerró la conexión.
						 *
						 * Primero lo eliminamos de epoll
						 * y después cerramos su FD.
						 */
						if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL,
								current_fd, NULL) < 0)
							perror("epoll_ctl del");

						close(current_fd);
					}
					else
					{
						printf("Recibido del cliente:\n%s\n",
							buffer);

						/*
						 * Respondemos igual que en tu código
						 * original.
						 */
						if (write(current_fd, hello,
								strlen(hello)) < 0)
							perror("write");
						else
						{
							printf("------------------"
								"Hello message sent"
								"-------------------\n");
						}

						/*
						 * =====================================
						 * ELIMINAR CLIENTE DE EPOLL
						 * =====================================
						 *
						 * Tu servidor original atendía
						 * un mensaje y después cerraba
						 * la conexión.
						 *
						 * Conservamos exactamente esa lógica.
						 *
						 * Como ya no queremos vigilar este
						 * descriptor:
						 *
						 *     EPOLL_CTL_DEL
						 *
						 * lo elimina de epoll.
						 */
						if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL,
								current_fd, NULL) < 0)
							perror("epoll_ctl del");

						close(current_fd);
					}
				}
			}
			i++;
		}
	}

	close(epoll_fd);
	close(server_fd);
	return (0);
}