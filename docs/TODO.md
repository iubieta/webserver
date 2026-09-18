
## [ServerCore.cpp](/src/ServerCore.cpp)

- [ ] temp no termina necesariamente en '\0' 
- [ ] en cleanConnectionn() no se comprueba posible error en epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL) []
- [ ] Destruir puntero de ls si falla el setup
- [ ] modificar orden de socks_.push y epoll_ctl en initSocket()
- [ ] cerrar epoll_fd cuando se llame el destructor
- [ ] poner sockets de conexiones (accept) en modo no bloquente
- [ ] Distincion de errores en recv y send en modo no bloqueante
- [ ] Investigar sobre SIGPIPE en errores de send
- [ ] Comprobar eficiencia a la hora de aceptar cliente
- [ ] retorno de setup en caso de error

## [ListeningSocket.cpp](/src/ListeningSocket.cpp)
- [ ] No instanciar con puerto invalido